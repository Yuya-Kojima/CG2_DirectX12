#include "Player.h"
#include "Actor/Level.h"
#include "Actor/Physics.h"
#include "Camera/GameCamera.h"
#include "Debug/Logger.h"
#include "Input/InputKeyState.h"
#include "Math/MathUtil.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Physics/CollisionSystem.h"
#include <format>
#include <algorithm>
#include <filesystem>
#include <format>

// 標準ライブラリのマクロ干渉を回避
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// ---------------------------------------------------------
// ライフサイクル
// ---------------------------------------------------------

Player::Player()
{
    // メンバ初期化子リストで初期化されるため空
}

Player::~Player()
{
    // 生成したObject3dを適切に破棄
    if (obj_) {
        delete obj_;
        obj_ = nullptr;
    }
}

// ---------------------------------------------------------
// 初期化
// ---------------------------------------------------------

void Player::Initialize(Input* input, Object3dRenderer* object3dRenderer)
{
    input_ = input;
    position_ = { 0.0f, 0.0f, 0.0f };

    // Object3dを生成し、描画システムへ登録
    if (object3dRenderer) {
        obj_ = new Object3d();
        obj_->Initialize(object3dRenderer);

        // モデルアセットの検索とロード
        // 優先的に拡張子付きファイル名を試し、クラス名などの生文字列でのResolve呼び出しを減らす
        std::vector<std::string> candidates = { "player.obj", "models/player.obj", "Player", "player" };
        bool loadedModel = false;
        for (const auto& c : candidates) {
            if (c.find('.') != std::string::npos || c.find('/') != std::string::npos || c.find('\\') != std::string::npos) {
                // ファイル名っぽい候補はそのまま解決を試みる
                std::string resolved = ModelManager::ResolveModelPath(c);
                if (resolved != c || std::filesystem::exists(resolved)) {
                    ModelManager::GetInstance()->LoadModel(resolved);
                    obj_->SetModel(resolved);
                    loadedModel = true;
                    break;
                }
            } else {
                // 生のクラス名などの場合は、まず小文字+拡張子で試す（不要な ResolveModelPath のログを減らすため）
                std::string lower = c;
                std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return std::tolower(ch); });
                std::vector<std::string> tryList = { lower + ".obj", lower + ".gltf", std::string("models/") + lower + ".obj" };
                for (const auto& t : tryList) {
                    std::string resolved = ModelManager::ResolveModelPath(t);
                    if (resolved != t || std::filesystem::exists(resolved)) {
                        ModelManager::GetInstance()->LoadModel(resolved);
                        obj_->SetModel(resolved);
                        loadedModel = true;
                        break;
                    }
                }
                if (loadedModel) break;
            }
        }

        // 描画オブジェクトの初期トランスフォームを設定
        obj_->SetTranslation(position_);
        obj_->SetScale({ 1.0f, 1.0f, 1.0f });
        obj_->SetRotation({ 0.0f, rotate_, 0.0f });
        try {
            Logger::Log(std::format("[プレイヤー] 初期化: pos=({:.2f},{:.2f},{:.2f})\n", position_.x, position_.y, position_.z));
        } catch (...) {
            Logger::Log(std::string("[プレイヤー] 初期化: pos=(") + std::to_string(position_.x) + "," + std::to_string(position_.y) + "," + std::to_string(position_.z) + ")\n");
        }
    }
}

// ---------------------------------------------------------
// フレーム更新処理
// ---------------------------------------------------------

void Player::Update(float dt)
{
    if (!input_)
        return;

    // --- 1. 移動入力の受付 ---
    float ax = 0.0f, az = 0.0f;
    if (input_->IsPressKey(DIK_A))
        ax -= 1.0f; // 左
    if (input_->IsPressKey(DIK_D))
        ax += 1.0f; // 右
    if (input_->IsPressKey(DIK_W))
        az += 1.0f; // 前
    if (input_->IsPressKey(DIK_S))
        az -= 1.0f; // 後

    float targetVX = ax * moveSpeed_;
    float targetVZ = az * moveSpeed_;

    // --- 2. 速度の計算（加減速処理） ---
    // X軸の加速・減速
    if (velocity_.x < targetVX) {
        velocity_.x = std::min(targetVX, velocity_.x + accel_ * dt);
    } else if (velocity_.x > targetVX) {
        velocity_.x = std::max(targetVX, velocity_.x - decel_ * dt);
    }

    // Z軸の加速・減速
    if (velocity_.z < targetVZ) {
        velocity_.z = std::min(targetVZ, velocity_.z + accel_ * dt);
    } else if (velocity_.z > targetVZ) {
        velocity_.z = std::max(targetVZ, velocity_.z - decel_ * dt);
    }

    // --- 3. ジャンプと重力の適用 ---
    /*if (onGround_ && input_->IsTriggerKey(DIK_SPACE)) {
        velocity_.y = jumpSpeed_;
        onGround_ = false;
    }*/
    velocity_.y -= gravity_ * dt;

    // --- 4. 水平方向の移動と衝突解決（スイープ＆スライド） ---
    Vector3 deltaXZ { velocity_.x * dt, 0.0f, velocity_.z * dt };

    if (level_ && (std::abs(deltaXZ.x) > 1e-6f || std::abs(deltaXZ.z) > 1e-6f)) {
        Vector3 remaining = deltaXZ; // フレーム内で移動すべき残り距離
        const int kMaxIters = 5; // 角などに挟まった際の脱出用反復回数
        Vector3 lastNormal { 0, 0, 0 }; // 最後に衝突した壁の法線

        for (int iter = 0; iter < kMaxIters; ++iter) {
            if (std::abs(remaining.x) < 1e-6f && std::abs(remaining.z) < 1e-6f)
                break;

            // 衝突判定を行うエリアの算出
            Vector3 endPos = { position_.x + remaining.x, position_.y, position_.z + remaining.z };
            Vector3 qmin { std::min(position_.x, endPos.x) - halfSize_, 0.0f, std::min(position_.z, endPos.z) - halfSize_ };
            Vector3 qmax { std::max(position_.x, endPos.x) + halfSize_, 0.0f, std::max(position_.z, endPos.z) + halfSize_ };

            std::vector<const Level::AABB*> candidates;
            level_->QueryWalls(qmin, qmax, candidates);

            // OBB も検索して候補に加える
            std::vector<const Level::OBB*> obbCandidates;
            level_->QueryOBBs(qmin, qmax, obbCandidates);

            float best_t = 1.0f;
            const Level::AABB* bestA = nullptr;
            const Level::OBB* bestO = nullptr;
            Vector3 bestN { 0, 0, 0 };

            for (const auto* a : candidates) {
                // すでに壁にめり込んでいる場合の即時解決
                float cx = std::max(a->min.x, std::min(position_.x, a->max.x));
                float cz = std::max(a->min.z, std::min(position_.z, a->max.z));
                float dx = position_.x - cx;
                float dz = position_.z - cz;
                if (dx * dx + dz * dz < halfSize_ * halfSize_) {
                    GamePhysics::ResolveSphereAabb2D(position_, halfSize_, a->min, a->max);
                    bestA = nullptr;
                    best_t = 1.0f;
                    continue;
                }

            // OBB 候補に対する処理
            for (const auto* o : obbCandidates) {
                // まずめり込みの即時解決
                // 近似的にローカルAABBでの最近接点を求める（回転は無視せず GamePhysics を使う）
                float cx = std::max(o->center.x - o->halfExtents.x, std::min(position_.x, o->center.x + o->halfExtents.x));
                float cz = std::max(o->center.z - o->halfExtents.z, std::min(position_.z, o->center.z + o->halfExtents.z));
                float dx = position_.x - cx;
                float dz = position_.z - cz;
                if (dx * dx + dz * dz < halfSize_ * halfSize_) {
                    GamePhysics::ResolveSphereObb2D(position_, halfSize_, o->center, o->halfExtents, o->yaw);
                    continue;
                }

                // スイープテスト
                float toi = 0.0f;
                Vector3 normal { 0, 0, 0 };
                if (GamePhysics::SweepSphereObb2D(position_, remaining, halfSize_, o->center, o->halfExtents, o->yaw, toi, normal)) {
                    if (toi >= 0.0f && toi < best_t) {
                        best_t = toi;
                        bestA = nullptr; // clear AABB
                        bestO = o;       // record OBB
                        bestN = normal;
                    }
                }
            }

                // スイープテスト（移動経路上の衝突確認）
                float toi = 0.0f;
                Vector3 normal { 0, 0, 0 };
            if (GamePhysics::SweepSphereAabb2D(position_, remaining, halfSize_, a->min, a->max, toi, normal)) {
                    if (toi >= 0.0f && toi < best_t) {
                        best_t = toi;
                        bestA = a;
                        bestN = normal;
                    }
                }
            }

            const float kCollisionEps = 1e-3f;
            if ((bestA || bestO) && best_t > kCollisionEps) {
                // 衝突地点まで進む
                position_.x += remaining.x * best_t;
                position_.z += remaining.z * best_t;

                // 残りの移動距離を壁に沿って滑らせる
                Vector3 left = { remaining.x * (1.0f - best_t), 0.0f, remaining.z * (1.0f - best_t) };
                Vector3 n = bestN;
                float nlen = std::sqrt(n.x * n.x + n.z * n.z);
                if (nlen > 1e-6f) {
                    n.x /= nlen;
                    n.z /= nlen;
                } else {
                    n.x = n.z = 0.0f;
                }

                // 壁方向の成分を除去
                float d = left.x * n.x + left.z * n.z;
                Vector3 slide = left;
                slide.x -= n.x * d;
                slide.z -= n.z * d;

                // 衝突対象ごとの再押し出し
                if (bestA) {
                    // AABBへの微調整
                    GamePhysics::ResolveSphereAabb2D(position_, halfSize_, bestA->min, bestA->max);
                } else if (bestO) {
                    // OBBへの微調整（回転を考慮）
                    GamePhysics::ResolveSphereObb2D(position_, halfSize_, bestO->center, bestO->halfExtents, bestO->yaw);
                }

                remaining = slide;
                lastNormal = n;
                continue;
            } else {
                // 衝突がなければそのまま移動
                position_.x += remaining.x;
                position_.z += remaining.z;
                // 移動後にOBBにめり込んでいないか念のためチェックして押し出す
                for (const auto* o : obbCandidates) {
                    // 最近接点による粗いチェック
                    float cx = std::max(o->center.x - o->halfExtents.x, std::min(position_.x, o->center.x + o->halfExtents.x));
                    float cz = std::max(o->center.z - o->halfExtents.z, std::min(position_.z, o->center.z + o->halfExtents.z));
                    float dx = position_.x - cx;
                    float dz = position_.z - cz;
                    if (dx * dx + dz * dz < halfSize_ * halfSize_) {
                        GamePhysics::ResolveSphereObb2D(position_, halfSize_, o->center, o->halfExtents, o->yaw);
                    }
                }

                remaining.x = remaining.z = 0.0f;
                break;
            }
        }

        // 速度ベクトルからも壁へ向かう成分を除去（滑る感覚を出す）
        if (std::abs(lastNormal.x) > 1e-6f || std::abs(lastNormal.z) > 1e-6f) {
            Vector3 velXZ { velocity_.x, 0.0f, velocity_.z };
            float vdot = velXZ.x * lastNormal.x + velXZ.z * lastNormal.z;
            velXZ.x -= lastNormal.x * vdot;
            velXZ.z -= lastNormal.z * vdot;
            velocity_.x = velXZ.x;
            velocity_.z = velXZ.z;
        }
    } else {
        // レベルがない、または移動が非常に小さい場合
        position_.x += deltaXZ.x;
        position_.z += deltaXZ.z;
    }

        // 最終移動後にレベルとのめり込みが発生していないかを必ず解消する
        // （OBB の候補検出が漏れた場合や、直前に設置されたオブジェクトで
        //  すり抜けが発生するのを防ぐための保険）
        if (level_) {
            level_->ResolveCollision(position_, halfSize_, true);
        }

        // --- 5. 垂直方向の更新と接地判定 ---
    position_.y += velocity_.y * dt;
    if (position_.y <= 0.0f) {
        position_.y = 0.0f;
        velocity_.y = 0.0f;
        onGround_ = true;
    }

    // --- 6. カメラ追従制御 ---
    if (camera_ && enableCameraFollow_) {
        Vector3 desired = { position_.x + followOffset_.x, position_.y + followOffset_.y, position_.z + followOffset_.z };
        Vector3 current = camera_->GetTranslate();
        Vector3 camPos;

        if (attachSmoothingActive_) {
            // スムーズなアタッチ補間（イーズアウト）
            attachSmoothingElapsed_ += dt;
            float alpha = attachSmoothingElapsed_ / std::max(0.0001f, attachDuration_);
            if (alpha >= 1.0f) {
                attachSmoothingActive_ = false;
                alpha = 1.0f;
            }
            float ease = 1.0f - (1.0f - alpha) * (1.0f - alpha);
            camPos.x = attachStart_.x + (attachTarget_.x - attachStart_.x) * ease;
            camPos.y = attachStart_.y + (attachTarget_.y - attachStart_.y) * ease;
            camPos.z = attachStart_.z + (attachTarget_.z - attachStart_.z) * ease;
        } else {
            // 通常時のLerp追従
            float t = std::min(1.0f, followSmooth_ * dt);
            camPos.x = current.x + (desired.x - current.x) * t;
            camPos.y = current.y + (desired.y - current.y) * t;
            camPos.z = current.z + (desired.z - current.z) * t;
        }

        // 壁ごしにカメラがめり込まないための簡易補正
        if (level_) {
            QueryParams qp;
            qp.start = current;
            qp.delta = Vector3 { camPos.x - current.x, 0.0f, camPos.z - current.z };
            qp.radius = 0.3f;
            qp.doSweep = true;
            qp.targets = QueryParams::TargetWall;
            qp.ignoreId = id_;
            auto qr = CollisionSystem::Query(qp, *level_);
            if (qr.anyHit) {
                camPos.x = current.x + (camPos.x - current.x) * 0.5f;
                camPos.z = current.z + (camPos.z - current.z) * 0.5f;
            }
        }

        camera_->SetTranslate(camPos);
        camera_->Update();
    }

    // --- 7. モデルデータの同期 ---
    if (obj_) {
        obj_->SetTranslation(position_);
        obj_->Update();
    }
}

// ---------------------------------------------------------
// 描画
// ---------------------------------------------------------

void Player::Draw()
{
    if (obj_) {
        obj_->Draw();
    }
}

// ---------------------------------------------------------
// 外部操作
// ---------------------------------------------------------

void Player::SetPosition(const Vector3& pos)
{
    position_ = pos;
    if (obj_) {
        obj_->SetTranslation(position_);
    }
}

void Player::AttachLevel(Level* level)
{
    level_ = level;
}

void Player::AttachCameraImmediate(GameCamera* cam)
{
    if (!cam)
        return;
    camera_ = cam;
    attachSmoothingActive_ = true;
    attachSmoothingElapsed_ = 0.0f;
    attachStart_ = camera_->GetTranslate();
    attachTarget_ = { position_.x + followOffset_.x, position_.y + followOffset_.y, position_.z + followOffset_.z };
    //enableCameraFollow_ = true;
}