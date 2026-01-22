#include "Player.h"
#include "Input/InputKeyState.h"
#include "Camera/GameCamera.h"
#include "Math/MathUtil.h"
#include "Object3d/Object3d.h"
#include "Model/ModelManager.h"
#include <algorithm>
#include <filesystem>
#include "Debug/Logger.h"
#include <format>
#include "Actor/Level.h"
#include "Actor/Physics.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// コンストラクタ
Player::Player() {}

// デストラクタ: Object3d を解放して D3D リソースを破棄する
Player::~Player() {
    if (obj_) {
        delete obj_;
        obj_ = nullptr;
    }
}

void Player::Initialize(InputKeyState* input, Object3dRenderer* object3dRenderer) {
    input_ = input;
    position_ = {0.0f, 0.0f, 0.0f};

    // Object3d を生成してレンダラに初期化する
    if (object3dRenderer) {
        obj_ = new Object3d();
        obj_->Initialize(object3dRenderer);

        // モデル候補をいくつか試して最初に見つかったものを割り当てる
        std::vector<std::string> candidates = {"Player", "player.obj", "models/player.obj", "player"};
        for (const auto &c : candidates) {
            std::string resolved = ModelManager::ResolveModelPath(c);
            if (resolved != c || std::filesystem::exists(resolved)) {
                try {
                    Logger::Log(std::format("Player: candidate='{}' resolved='{}'\n", c, resolved));
                } catch (...) {}
                ModelManager::GetInstance()->LoadModel(resolved);
                obj_->SetModel(resolved);
                try {
                    Logger::Log(std::format("Player: assigned model '{}'.\n", resolved));
                } catch (...) {}
                break;
            }
        }

        // 表示されるように初期変換を設定
        obj_->SetTranslation(position_);
        obj_->SetScale({1.0f, 1.0f, 1.0f});
        obj_->SetRotation({0.0f, rotate_, 0.0f});
    }
}

void Player::Update(float dt) {
    if (!input_) return;

    // 入力の取得: X 軸（左右）と Z 軸（前後）
    float ax = 0.0f, az = 0.0f;
    if (input_->IsPressKey(DIK_A)) ax -= 1.0f; // 左へ
    if (input_->IsPressKey(DIK_D)) ax += 1.0f; // 右へ
    if (input_->IsPressKey(DIK_W)) az += 1.0f; // 前へ
    if (input_->IsPressKey(DIK_S)) az -= 1.0f; // 後へ

    float targetVX = ax * moveSpeed_;
    float targetVZ = az * moveSpeed_;

    // 速度の更新: 加速度・減速度を考慮して目標速度に近づける
    // X 方向
    if (velocity_.x < targetVX) {
        velocity_.x = std::min(targetVX, velocity_.x + accel_ * dt);
    } else if (velocity_.x > targetVX) {
        velocity_.x = std::max(targetVX, velocity_.x - decel_ * dt);
    }

    // Z 方向
    if (velocity_.z < targetVZ) {
        velocity_.z = std::min(targetVZ, velocity_.z + accel_ * dt);
    } else if (velocity_.z > targetVZ) {
        velocity_.z = std::max(targetVZ, velocity_.z - decel_ * dt);
    }

    // ジャンプ処理: 接地中にスペースで垂直速度を与える
    if (onGround_ && input_->IsTriggerKey(DIK_SPACE)) {
        velocity_.y = jumpSpeed_;
        onGround_ = false;
    }

    // 重力適用
    velocity_.y -= gravity_ * dt;

    // 水平移動: 反復スイープ&スライドで複数衝突を処理する
    Vector3 deltaXZ { velocity_.x * dt, 0.0f, velocity_.z * dt };
    if (level_ && (std::abs(deltaXZ.x) > 1e-6f || std::abs(deltaXZ.z) > 1e-6f)) {
        Vector3 remaining = deltaXZ;
        const int kMaxIters = 3;
        Vector3 lastNormal {0,0,0};
        for (int iter = 0; iter < kMaxIters; ++iter) {
            if (std::abs(remaining.x) < 1e-6f && std::abs(remaining.z) < 1e-6f) break;

            Vector3 endPos = { position_.x + remaining.x, position_.y, position_.z + remaining.z };
            Vector3 qmin { std::min(position_.x, endPos.x) - halfSize_, 0.0f, std::min(position_.z, endPos.z) - halfSize_ };
            Vector3 qmax { std::max(position_.x, endPos.x) + halfSize_, 0.0f, std::max(position_.z, endPos.z) + halfSize_ };
            std::vector<const Level::AABB*> candidates;
            level_->QueryWalls(qmin, qmax, candidates);

            float best_t = 1.0f;
            const Level::AABB* bestA = nullptr;
            Vector3 bestN {0,0,0};

            // 探索: 侵入なら解消、それ以外は最短ヒットを見つける
            for (const auto *a : candidates) {
                float cx = std::max(a->min.x, std::min(position_.x, a->max.x));
                float cz = std::max(a->min.z, std::min(position_.z, a->max.z));
                float dx = position_.x - cx;
                float dz = position_.z - cz;
                float dist2 = dx*dx + dz*dz;
                if (dist2 < halfSize_ * halfSize_) {
                    // 既にめり込んでいる -> 押し出して次の反復で再評価
                    GamePhysics::ResolveSphereAabb2D(position_, halfSize_, a->min, a->max);
                    bestA = nullptr; best_t = 1.0f; // reset
                    continue;
                }

                float toi = 0.0f;
                Vector3 normal {0,0,0};
                if (GamePhysics::SweepSphereAabb2D(position_, remaining, halfSize_, a->min, a->max, toi, normal)) {
                    if (toi >= 0.0f && toi < best_t) {
                        best_t = toi; bestA = a; bestN = normal;
                    }
                }
            }

            const float kCollisionEps = 1e-4f;
            if (bestA && best_t > kCollisionEps) {
                // 衝突まで移動
                position_.x += remaining.x * best_t;
                position_.z += remaining.z * best_t;

                // 残りをスライド成分へ射影
                Vector3 left = { remaining.x * (1.0f - best_t), 0.0f, remaining.z * (1.0f - best_t) };
                Vector3 n = bestN;
                float nlen = std::sqrt(n.x*n.x + n.z*n.z);
                if (nlen > 1e-6f) { n.x /= nlen; n.z /= nlen; } else { n.x = n.z = 0.0f; }
                float d = left.x * n.x + left.z * n.z;
                Vector3 slide = left; slide.x -= n.x * d; slide.z -= n.z * d;

                // 押し出し微調整
                GamePhysics::ResolveSphereAabb2D(position_, halfSize_, bestA->min, bestA->max);

                // 次の反復でスライド量を処理
                remaining = slide;
                lastNormal = n;
                continue; // 次反復
            } else {
                // 衝突無しまたは微小衝突 -> 残りを適用して終了
                position_.x += remaining.x;
                position_.z += remaining.z;
                remaining.x = remaining.z = 0.0f;
                break;
            }
        }

        // 最後に速度から法線成分を取り除き滑る速度を残す
        if (std::abs(lastNormal.x) > 1e-6f || std::abs(lastNormal.z) > 1e-6f) {
            Vector3 velXZ { velocity_.x, 0.0f, velocity_.z };
            float vdot = velXZ.x * lastNormal.x + velXZ.z * lastNormal.z;
            velXZ.x -= lastNormal.x * vdot; velXZ.z -= lastNormal.z * vdot;
            if (std::abs(velXZ.x) < 1e-3f) velXZ.x = 0.0f; if (std::abs(velXZ.z) < 1e-3f) velXZ.z = 0.0f;
            velocity_.x = velXZ.x; velocity_.z = velXZ.z;
        }
    } else {
        // Level がないまたは移動量がゼロなら通常移動
        position_.x += deltaXZ.x;
        position_.z += deltaXZ.z;
    }

    // 垂直方向の移動は通常どおり
    position_.y += velocity_.y * dt;

    // 地面との衝突判定（Y=0 面）
    if (position_.y <= 0.0f) {
        position_.y = 0.0f;
        velocity_.y = 0.0f;
        onGround_ = true;
    }

    // カメラ追従処理
    if (camera_) {
        Vector3 desired { position_.x - 6.0f, position_.y + 8.0f, position_.z - 6.0f };
        Vector3 current = camera_->GetTranslate();
        const float smooth = 10.0f;
        float t = std::min(1.0f, smooth * dt);
        Vector3 camPos;
        camPos.x = current.x + (desired.x - current.x) * t;
        camPos.y = current.y + (desired.y - current.y) * t;
        camPos.z = current.z + (desired.z - current.z) * t;
        camera_->SetTranslate(camPos);
        camera_->Update();
    }

    // Object3d の更新
    if (obj_) {
        obj_->SetTranslation(position_);
        obj_->Update();
    }

    // (debug logging removed)
}

void Player::Draw() {
    if (obj_) {
        obj_->Draw();
    }
}

void Player::SetPosition(const Vector3& pos) {
    position_ = pos;
    if (obj_) {
        obj_->SetTranslation(position_);
    }
}

void Player::AttachLevel(Level* level) {
    level_ = level;
}
