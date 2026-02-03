#include "Actor/Npc.h"
#include "Actor/Level.h"
#include "Actor/Physics.h"
#include "Camera/GameCamera.h"
#include "Model/ModelManager.h"
#include "Model/Model.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
#include "Debug/Logger.h"
#include "Math/MathUtil.h"
#include <algorithm>
#include <cmath>
#include <vector>

// Windows環境等で競合する可能性があるため、min/maxマクロを無効化
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

void Npc::Initialize(Object3dRenderer* renderer, const Vector3& startPos)
{
    renderer_ = renderer;
    pos_ = startPos;
    // record spawn position for delivery tasks
    spawnPos_ = startPos;

    // 描画オブジェクトの生成と初期化
    obj_ = std::make_unique<Object3d>();
    obj_->Initialize(renderer_);

    // モデルマネージャを介してNPC用モデルをロードし適用
    ModelManager::GetInstance()->LoadModel("NPC.obj");
    obj_->SetModel("NPC.obj");

    // スケール調整と初期位置の設定
    obj_->SetScale({ 1.0f, 1.0f, 1.0f });
    obj_->SetRotation({ 0.0f, 1.5f, 0.0f });
    obj_->SetTranslation(pos_);
    try {
        Logger::Log(std::format("[Npc] Initialize: pos=({:.2f},{:.2f},{:.2f})\n", pos_.x, pos_.y, pos_.z));
    } catch (...) {
        Logger::Log(std::string("[Npc] Initialize: pos=(") + std::to_string(pos_.x) + "," + std::to_string(pos_.y) + "," + std::to_string(pos_.z) + ")\n");
    }

    // package object: optional carried item. Load model but do not fail if missing.
    packageObj_ = std::make_unique<Object3d>();
    packageObj_->Initialize(renderer_);
    // Try to load a package model file; it's okay if not found — ModelManager will log.
    ModelManager::GetInstance()->LoadModel("NPC_carry.obj");
    try {
        packageObj_->SetModel("NPC_carry.obj");
        packageObj_->SetScale({ 1.0f, 1.0f, 1.0f });
    } catch (...) {
        // If SetModel asserts in debug, we ignore here to allow builds without the model.
    }
    // Initially carrying as requested by user — set true so package appears
    carrying_ = true;
    // set initial package translation
    Vector3 packagePos = { pos_.x + packageOffset_.x, pos_.y + packageOffset_.y, pos_.z + packageOffset_.z };
    packageObj_->SetTranslation(packagePos);
}

// Begin returning to spawn (used after reaching goal)
void Npc::BeginReturnToSpawn() {
    returning_ = true;
    // set straight direction toward spawn
    Vector3 dir = { spawnPos_.x - pos_.x, 0.0f, spawnPos_.z - pos_.z };
    float len = std::sqrt(dir.x*dir.x + dir.z*dir.z);
    if (len > 1e-6f) { dir.x /= len; dir.z /= len; }
    else { dir = { 0.0f, 0.0f, 1.0f }; }
    straightDir_ = dir;
    state_ = State::Straight;
    // drop carried item when returning
    carrying_ = false;

    // compute desired facing (yaw) toward spawn and store for smooth rotation
    targetYaw_ = std::atan2(dir.x, dir.z);
}

bool Npc::HasReturnedToSpawn() const {
    float dx = pos_.x - spawnPos_.x;
    float dz = pos_.z - spawnPos_.z;
    return (dx*dx + dz*dz) <= (desiredDistance_ * desiredDistance_);
}

void Npc::Update(float dt, const Vector3& targetPos, Level& level)
{
    // Simple state: Straight movement tries to step onto OBB tops when detected.
    if (state_ == State::Straight) {
        // ensure straightDir is normalized in XZ
        Vector3 dir = { straightDir_.x, 0.0f, straightDir_.z };
        dir = SafeNormalize(dir);

        // desired velocity
        vel_.x = dir.x * moveSpeed_;
        vel_.z = dir.z * moveSpeed_;

        // probe point a little ahead — use a predicted position (pos + vel * dt)
        // to avoid detecting obstacles based on the current frame's pre-collision
        // position which may cause small oscillations (step forward then pushed
        // back by collision resolve). Predicting a bit ahead reduces those false
        // positives.
        Vector3 predictedPos = { pos_.x + vel_.x * dt, pos_.y + vel_.y * dt, pos_.z + vel_.z * dt };
        // Reduce lookahead so detection happens closer to NPC
        Vector3 probeEnd = { predictedPos.x + dir.x * 0.08f, predictedPos.y + 0.1f, predictedPos.z + dir.z * 0.08f };

        // query nearby walls and obb
        Vector3 qmin { std::min(pos_.x, probeEnd.x) - kDefaultRadius, 0.0f, std::min(pos_.z, probeEnd.z) - kDefaultRadius };
        Vector3 qmax { std::max(pos_.x, probeEnd.x) + kDefaultRadius, 2.0f, std::max(pos_.z, probeEnd.z) + kDefaultRadius };

        std::vector<const Level::AABB*> aabbCandidates;
        level.QueryWalls(qmin, qmax, aabbCandidates);
        std::vector<const Level::OBB*> obbCandidates;
        level.QueryOBBs(qmin, qmax, obbCandidates);

        bool handled = false;

        // Debug: log probe point and candidate counts
        try {
            Logger::Log(std::format("[Npc] Straight probeEnd=({:.3f},{:.3f},{:.3f}) aabbCandidates={} obbCandidates={}\n",
                probeEnd.x, probeEnd.y, probeEnd.z, (int)aabbCandidates.size(), (int)obbCandidates.size()));
        } catch(...) {}

        // Prefer OBB mounting: if probeEnd projects over OBB conservative area and raycast down hits top, mount
        // But skip mounting while in post-fall cooldown or actively falling from a mount to avoid immediate re-mounts
        if (postFallCooldown_ <= 0.0f && !fallingFromMount_) {
            for (const Level::OBB* o : obbCandidates) {
                float cy = std::cos(o->yaw);
                float sy = std::sin(o->yaw);
                float absCy = std::fabs(cy);
                float absSy = std::fabs(sy);
                float extX = absCy * o->halfExtents.x + absSy * o->halfExtents.z;
                float extZ = absSy * o->halfExtents.x + absCy * o->halfExtents.z;

                // Debug: log OBB candidate info
                try {
                    Logger::Log(std::format("[Npc] OBB cand ownerId={} center=({:.3f},{:.3f},{:.3f}) half=({:.3f},{:.3f},{:.3f}) yaw={:.3f} extX={:.3f} extZ={:.3f}\n",
                        o->ownerId, o->center.x, o->center.y, o->center.z, o->halfExtents.x, o->halfExtents.y, o->halfExtents.z, o->yaw, extX, extZ));
                } catch(...) {}

                // allow a small margin so NPC can detect edges even when probeEnd is slightly outside
                const float probeMargin = 0.3f;
                if (probeEnd.x < o->center.x - extX - probeMargin || probeEnd.x > o->center.x + extX + probeMargin) {
                    try { Logger::Log(std::format("[Npc] probeEnd.x outside extents (x={:.3f}, min={:.3f}, max={:.3f})\n", probeEnd.x, o->center.x - extX, o->center.x + extX)); } catch(...) {}
                    continue;
                }
                if (probeEnd.z < o->center.z - extZ - probeMargin || probeEnd.z > o->center.z + extZ + probeMargin) {
                    try { Logger::Log(std::format("[Npc] probeEnd.z outside extents (z={:.3f}, min={:.3f}, max={:.3f})\n", probeEnd.z, o->center.z - extZ, o->center.z + extZ)); } catch(...) {}
                    continue;
                }

                // Sample multiple points across NPC footprint to detect face-vs-face contact.
                const float sampleTol = 0.35f;
                const float obbTopY = o->center.y + o->halfExtents.y;
                const float samplesXZ[5][2] = {
                    { 0.0f, 0.0f },
                    { dir.x * kDefaultRadius, dir.z * kDefaultRadius },
                    { -dir.x * kDefaultRadius, -dir.z * kDefaultRadius },
                    { -dir.z * kDefaultRadius, dir.x * kDefaultRadius },
                    { dir.z * kDefaultRadius, -dir.x * kDefaultRadius }
                };
                float foundHitY = 0.0f;
                float foundHitX = 0.0f;
                float foundHitZ = 0.0f;
                bool anyHit = false;
                for (int si = 0; si < 5; ++si) {
                    // sample around the probeEnd (forward area) instead of NPC center
                    float sx = probeEnd.x + samplesXZ[si][0];
                    float sz = probeEnd.z + samplesXZ[si][1];
                    float sampleHitY = 0.0f;
                    if (!level.RaycastDown({ sx, probeEnd.y + kStepHeight + kStepExtra + 0.5f, sz }, kStepHeight + kStepExtra + 0.5f, sampleHitY)) {
                        try { Logger::Log(std::format("[Npc] sample {} miss at ({:.2f},{:.2f})\n", si, sx, sz)); } catch(...) {}
                        continue;
                    }
                    try { Logger::Log(std::format("[Npc] sample {} hit at ({:.2f},{:.2f}) hitY={:.3f}\n", si, sx, sz, sampleHitY)); } catch(...) {}
                    // check vertical match with obb top
                    if (std::fabs(sampleHitY - obbTopY) > sampleTol)
                        continue;
                    // ensure sample point projects into conservative extents
                    if (sx < o->center.x - extX || sx > o->center.x + extX || sz < o->center.z - extZ || sz > o->center.z + extZ)
                        continue;
                    foundHitY = sampleHitY;
                    foundHitX = sx;
                    foundHitZ = sz;
                    anyHit = true;
                    break;
                }
                if (anyHit) {
                    float stepH = foundHitY - pos_.y;
                    // allow a larger climb tolerance: include step extra and some margin
                    const float maxClimb = kStepHeight + kStepExtra + 0.6f;
                    const float minDrop = -0.25f; // allow small drops
                    if (stepH >= minDrop && stepH <= maxClimb) {
                        float oy = o->yaw;
                        float ax = std::sin(oy);
                        float az = std::cos(oy);
                        // snap horizontal position to the sample hit so NPC stands on top
                        pos_.x = foundHitX;
                        pos_.z = foundHitZ;
                        // raise Y a bit more so model doesn't intersect with top surface
                        // Add a small extra raise proportional to the OBB half-height to handle
                        // cases where the model was scaled up and the original offset is too low.
                        const float mountRaise = 0.08f;
                        const float obbExtraRaise = std::max(0.0f, o->halfExtents.y * 0.1f);
                        pos_.y = foundHitY + mountOffsetY_ + mountRaise + obbExtraRaise;
                        if (obj_) obj_->SetTranslation(pos_);
                        vel_.y = 0.0f;
                        mounted_ = true;
                        mountTimer_ = 0.0f;
                        mountedOwnerId_ = o->ownerId;
                        // store obb parameters to allow edge-aware unmounting
                        mountedObbCenter_ = o->center;
                        mountedObbHalfExtents_ = o->halfExtents;
                        mountedObbYaw_ = o->yaw;
                        straightDir_.x = ax; straightDir_.z = az;
                        vel_.x = ax * moveSpeed_; vel_.z = az * moveSpeed_;
                        handled = true;
                        try { Logger::Log(std::format("[Npc] Mounted on OBB ownerId={} sampleHit=({:.2f},{:.2f})\n", o->ownerId, pos_.x, pos_.z)); } catch(...) {}
                        break;
                    } else {
                        try { Logger::Log(std::format("[Npc] stepH out of range: stepH={:.3f} allowed=[{:.3f},{:.3f}]\n", stepH, minDrop, maxClimb)); } catch(...) {}
                    }
                }
            }
        }

        if (!handled) {
            // Prefer sliding along OBB obstacles (e.g. sticks) using their yaw
            for (const Level::OBB* o : obbCandidates) {
                // conservative extents in XZ after rotation
                float cy = std::cos(o->yaw);
                float sy = std::sin(o->yaw);
                float absCy = std::fabs(cy);
                float absSy = std::fabs(sy);
                float extX = absCy * o->halfExtents.x + absSy * o->halfExtents.z;
                float extZ = absSy * o->halfExtents.x + absCy * o->halfExtents.z;

                // tighten margin to require closer proximity before steering along OBB
                const float probeMargin = 0.02f;
                // require the OBB center to be reasonably close to the NPC before considering steering
                const float detectDist = 1.25f; // world units
                float dxC = o->center.x - pos_.x;
                float dzC = o->center.z - pos_.z;
                if (dxC * dxC + dzC * dzC > detectDist * detectDist) {
                    continue; // too far, skip this OBB
                }
                if (probeEnd.x >= o->center.x - extX - probeMargin && probeEnd.x <= o->center.x + extX + probeMargin
                    && probeEnd.z >= o->center.z - extZ - probeMargin && probeEnd.z <= o->center.z + extZ + probeMargin) {
                    // steer along the OBB's forward (yaw) direction
                    Vector3 newDir = { std::sin(o->yaw), 0.0f, std::cos(o->yaw) };
                    // preserve forward sign relative to previous straightDir_
                    float dot = newDir.x * straightDir_.x + newDir.z * straightDir_.z;
                    if (dot < 0.0f) { newDir.x = -newDir.x; newDir.z = -newDir.z; }
                    newDir = SafeNormalize(newDir);
                    straightDir_ = newDir;
                    vel_.x = straightDir_.x * moveSpeed_;
                    vel_.z = straightDir_.z * moveSpeed_;
                    if (obj_) {
                        float yaw = std::atan2(vel_.x, vel_.z);
                        Vector3 rot = obj_->GetRotation(); rot.y = yaw; obj_->SetRotation(rot);
                    }
                    handled = true;
                    break;
                }
            }

            // check simple AABB blocking
            for (const Level::AABB* box : aabbCandidates) {
                if (probeEnd.x >= box->min.x && probeEnd.x <= box->max.x && probeEnd.z >= box->min.z && probeEnd.z <= box->max.z) {
                    // Instead of stopping, change travel axis to run along the obstacle
                    // Compute conservative box center in XZ
                    float bx = 0.5f * (box->min.x + box->max.x);
                    float bz = 0.5f * (box->min.z + box->max.z);
                    float dx = pos_.x - bx;
                    float dz = pos_.z - bz;

                    // Decide which axis to align to based on which separation is larger
                    Vector3 newDir = {0.0f, 0.0f, 0.0f};
                    if (std::fabs(dx) > std::fabs(dz)) {
                        // obstacle lies more to left/right -> run along Z axis
                        // choose sign that preserves forward component of previous direction
                        newDir.z = (straightDir_.z >= 0.0f) ? 1.0f : -1.0f;
                    } else {
                        // obstacle lies more to front/back -> run along X axis
                        newDir.x = (straightDir_.x >= 0.0f) ? 1.0f : -1.0f;
                    }

                    // normalize newDir (it's unit on chosen axis)
                    newDir = SafeNormalize(newDir);

                    straightDir_ = newDir;
                    vel_.x = straightDir_.x * moveSpeed_;
                    vel_.z = straightDir_.z * moveSpeed_;

                    // apply rotation to model so it faces movement direction
                    if (obj_) {
                        float yaw = std::atan2(vel_.x, vel_.z);
                        Vector3 rot = obj_->GetRotation(); rot.y = yaw; obj_->SetRotation(rot);
                    }

                    handled = true;
                    break;
                }
            }
        }

        // integrate horizontal movement
        // if in post-fall cooldown, stop horizontal progress
        if (postFallCooldown_ > 0.0f) {
            // decay cooldown
            postFallCooldown_ = std::max(0.0f, postFallCooldown_ - dt);
            // zero horizontal velocity so NPC doesn't move
            vel_.x = 0.0f; vel_.z = 0.0f;
        }

        pos_.x += vel_.x * dt;
        pos_.z += vel_.z * dt;

        // handle mount: stay mounted while still colliding with a blocking AABB in front (e.g., a stick)
        if (mounted_) {
            // probe a bit ahead in facing direction to detect a blocking AABB
            Vector3 mdir = { straightDir_.x, 0.0f, straightDir_.z };
            mdir = SafeNormalize(mdir);
            Vector3 forwardProbe = { pos_.x + mdir.x * 0.5f, pos_.y, pos_.z + mdir.z * 0.5f };
            Vector3 qminProbe { std::min(pos_.x, forwardProbe.x) - kDefaultRadius, 0.0f, std::min(pos_.z, forwardProbe.z) - kDefaultRadius };
            Vector3 qmaxProbe { std::max(pos_.x, forwardProbe.x) + kDefaultRadius, 2.0f, std::max(pos_.z, forwardProbe.z) + kDefaultRadius };
            std::vector<const Level::AABB*> nearAABBs;
            level.QueryWalls(qminProbe, qmaxProbe, nearAABBs);
            bool hasStick = false;
            for (const Level::AABB* box : nearAABBs) {
                if (forwardProbe.x >= box->min.x && forwardProbe.x <= box->max.x && forwardProbe.z >= box->min.z && forwardProbe.z <= box->max.z) {
                    hasStick = true;
                    break;
                }
            }
            // determine if NPC is still above the mounted OBB (in local OBB XZ extents)
            float cyo = std::cos(mountedObbYaw_);
            float syo = std::sin(mountedObbYaw_);
            float lx =  cyo * (pos_.x - mountedObbCenter_.x) + syo * (pos_.z - mountedObbCenter_.z);
            float lz = -syo * (pos_.x - mountedObbCenter_.x) + cyo * (pos_.z - mountedObbCenter_.z);
            const float stayMargin = 0.12f; // allow larger margin so we don't start unmounting early
            bool stillOnObb = (lx >= -mountedObbHalfExtents_.x - stayMargin && lx <= mountedObbHalfExtents_.x + stayMargin
                               && lz >= -mountedObbHalfExtents_.z - stayMargin && lz <= mountedObbHalfExtents_.z + stayMargin);
            try {
                Logger::Log(std::format("[Npc] mount check hasStick={} stillOnObb={} pos=({:.3f},{:.3f},{:.3f}) mountTimer={:.3f}\n",
                    hasStick ? 1 : 0, stillOnObb ? 1 : 0, pos_.x, pos_.y, pos_.z, mountTimer_));
            } catch(...) {}

            if (hasStick || stillOnObb) {
                // reset grace timer while still touching stick OR still clearly on top of OBB
                if (mountTimer_ > 0.0f) {
                    try { Logger::Log(std::format("[Npc] mountTimer reset (hasStick||stillOnObb)\n")); } catch(...) {}
                }
                mountTimer_ = 0.0f;
            } else {
                // Immediately unmount when stick is gone. Start a gentle descent (no hard snap).
                mounted_ = false;
                mountedOwnerId_ = 0;
                mountTimer_ = 0.0f;
                // begin smooth falling: slightly quicker descent
                fallingFromMount_ = true;
                vel_.y = -0.60f; // initial downward velocity (quicker)
                try { Logger::Log(std::format("[Npc] Immediate unmount (stick lost), begin gentle fall, pos=({:.3f},{:.3f},{:.3f})\n", pos_.x, pos_.y, pos_.z)); } catch(...) {}
            }
        }

        // simple collision resolve and bounds
        level.ResolveCollision(pos_, kDefaultRadius, true);
        level.KeepInsideBounds(pos_, kDefaultRadius);

        if (!mounted_) {
            float groundY = 0.0f;
            if (fallingFromMount_) {
                // Smooth falling: integrate vertical velocity with gentle gravity
                const float gravity = -6.0f;         // m/s^2, a bit stronger for quicker descent
                const float terminalVel = -3.5f;    // limit fall speed (more than before)

                // integrate
                vel_.y = std::max(terminalVel, vel_.y + gravity * dt);
                pos_.y += vel_.y * dt;

                // check for ground near current position; if close enough, snap and finish falling
                if (level.RaycastDown({ pos_.x, pos_.y + 0.5f, pos_.z }, 50.0f, groundY)) {
                    const float landingThreshold = 0.04f; // slightly larger threshold for earlier snap
                    if (pos_.y <= groundY + landingThreshold) {
                        pos_.y = groundY;
                        fallingFromMount_ = false;
                        vel_.y = 0.0f;
                        // start cooldown to prevent immediate forward movement
                        postFallCooldown_ = kPostFallCooldown;
                        // stop current velocity; keep straightDir_ so NPC can resume after cooldown
                        vel_.x = 0.0f; vel_.z = 0.0f;
                        // resume Straight state; movement will remain disabled while postFallCooldown_ > 0
                        state_ = State::Straight;
                    }
                }
            } else {
                if (level.RaycastDown({ pos_.x, pos_.y + kStepHeight + kStepExtra, pos_.z }, kStepHeight + kStepExtra + 1.0f, groundY)) {
                    pos_.y = groundY;
                }
            }
        }

        // Smoothly interpolate facing yaw toward targetYaw_ only when returning.
        if (obj_) {
            if (returning_) {
                float currentYaw = obj_->GetRotation().y;
                const float PI = 3.14159265f;
                float delta = targetYaw_ - currentYaw;
                while (delta > PI) delta -= 2.0f * PI;
                while (delta < -PI) delta += 2.0f * PI;
                float maxStep = yawSmoothSpeed_ * dt;
                float step = std::max(-maxStep, std::min(maxStep, delta));
                float newYaw = currentYaw + step;
                Vector3 rot = obj_->GetRotation(); rot.y = newYaw; obj_->SetRotation(rot);
            }
            // always update translation and object
            obj_->SetTranslation(pos_);
            obj_->Update();
        }
    // update package transform to follow NPC when carrying
    if (carrying_ && packageObj_) {
        Vector3 packagePos = { pos_.x + packageOffset_.x, pos_.y + packageOffset_.y, pos_.z + packageOffset_.z };
        packageObj_->SetTranslation(packagePos);
        // face same rotation as NPC
        packageObj_->SetRotation(obj_ ? obj_->GetRotation() : Vector3{0.0f,0.0f,0.0f});
        packageObj_->Update();
    }
        return;
    }

    // MoveToTarget behaviour removed: scene currently uses Straight/Idle only.
    // If future pathfinding/arrive is needed, reintroduce logic here.
}

void Npc::Draw()
{
    // 3Dオブジェクトの描画実行
    if (obj_)
        obj_->Draw();
    // draw carried package after NPC so it appears on top
    if (carrying_ && packageObj_)
        packageObj_->Draw();
}