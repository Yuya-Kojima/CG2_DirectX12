#include "Actor/Npc.h"
#include "Actor/Level.h"
#include "Actor/Physics.h"
#include "Camera/GameCamera.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
#include "Debug/Logger.h"
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

    // 描画オブジェクトの生成と初期化
    obj_ = std::make_unique<Object3d>();
    obj_->Initialize(renderer_);

    // モデルマネージャを介してNPC用モデルをロードし適用
    ModelManager::GetInstance()->LoadModel("breakableBlock.obj");
    obj_->SetModel("breakableBlock.obj");

    // スケール調整と初期位置の設定
    obj_->SetScale({ 0.6f, 0.6f, 0.6f });
    obj_->SetTranslation(pos_);
    try {
        Logger::Log(std::format("[Npc] Initialize: pos=({:.2f},{:.2f},{:.2f})\n", pos_.x, pos_.y, pos_.z));
    } catch (...) {
        Logger::Log(std::string("[Npc] Initialize: pos=(") + std::to_string(pos_.x) + "," + std::to_string(pos_.y) + "," + std::to_string(pos_.z) + ")\n");
    }
}

void Npc::Update(float dt, const Vector3& targetPos, Level& level)
{
    // --- 1. 到達行動 (Arrive) の計算 ---
    // ターゲットまでの相対ベクトルと距離を算出
    float tx = targetPos.x - pos_.x;
    float tz = targetPos.z - pos_.z;
    float dist = std::sqrt(tx * tx + tz * tz);

    float dirX = 0.0f, dirZ = 0.0f;
    // ゼロ除算防止チェック
    if (dist > 0.0001f) {
        dirX = tx / dist;
        dirZ = tz / dist;
    }

    // 目的地周辺でのスムーズな減速処理
    float targetSpeed = moveSpeed_;
    if (dist < slowRadius_) {
        // 距離に応じて速度を線形に落とす
        targetSpeed = moveSpeed_ * (dist / slowRadius_);
        // 停止許容範囲内なら速度を0にする
        if (dist < desiredDistance_)
            targetSpeed = 0.0f;
    }

    // 目標速度ベクトルを算出
    float targetVX = dirX * targetSpeed;
    float targetVZ = dirZ * targetSpeed;

    // X軸の加減速: 現在の速度を目標速度に近づける
    if (vel_.x < targetVX)
        vel_.x = std::min(targetVX, vel_.x + accel_ * dt);
    else
        vel_.x = std::max(targetVX, vel_.x - decel_ * dt);

    // Z軸の加減速
    if (vel_.z < targetVZ)
        vel_.z = std::min(targetVZ, vel_.z + accel_ * dt);
    else
        vel_.z = std::max(targetVZ, vel_.z - decel_ * dt);

    // --- 追加: 速度ベクトルの大きさを最大速度で制限する ---
    // 斜め移動で速度が moveSpeed_ を超えないようにクランプする
    float horizSpeed = std::sqrt(vel_.x * vel_.x + vel_.z * vel_.z);
    if (horizSpeed > moveSpeed_ && horizSpeed > kSmallEpsilon) {
        float s = moveSpeed_ / horizSpeed;
        vel_.x *= s;
        vel_.z *= s;
    }

    // --- 2. 経路の再計画 (Path Re-planning) ---
    replanTimer_ -= dt;
    // 計算負荷を抑えるため、一定間隔ごとに実行。ただし開始/目標タイルが変わらない場合は再検索をスキップする
    if (nav_ && replanTimer_ <= 0.0f) {
        int sx, sz, gx, gz;
        if (nav_->WorldToTile(pos_, sx, sz) && nav_->WorldToTile(targetPos, gx, gz)) {
            // 前回と開始・目標タイルが同じなら再計算をスキップ
            if (sx != lastSearchStartX_ || sz != lastSearchStartZ_ || gx != lastSearchGoalX_ || gz != lastSearchGoalZ_) {
                // A*などのアルゴリズムで経路リストを生成
                path_ = nav_->FindPath(sx, sz, gx, gz);
                pathIndex_ = 0;

                // 前回検索タイル位置を更新
                lastSearchStartX_ = sx;
                lastSearchStartZ_ = sz;
                lastSearchGoalX_ = gx;
                lastSearchGoalZ_ = gz;
            }
        }
        replanTimer_ = kReplanInterval; // タイマーリセット（定数を使用）
    }

    // --- 3. ハザード（危険地帯）の早期検知と回避 ---
    // 次のフレームで移動する予定の座標を計算
    Vector3 next { pos_.x + vel_.x * dt, 0.0f, pos_.z + vel_.z * dt };
    // 当たり判定半径には定数を使用
    if (level.IsHazardHit(next, kDefaultRadius)) {
        // 危険地帯に触れる場合は速度を大幅に減衰
        vel_.x *= 0.2f;
        vel_.z *= 0.2f;
        // 進行方向に対して垂直なベクトルを計算し、横に避ける動きを加える
        float avoidX = -dirZ;
        float avoidZ = dirX;
        pos_.x += avoidX * moveSpeed_ * 0.3f * dt;
        pos_.z += avoidZ * moveSpeed_ * 0.3f * dt;
    }

    // --- 4. 壁に対するスイープ移動と衝突解決 (2D/XZ平面) ---
    // NPCの当たり判定半径
    const float radius = kDefaultRadius;
    Vector3 start = pos_;
    Vector3 delta { vel_.x * dt, 0.0f, vel_.z * dt };

    // 移動経路を包含する矩形領域(AABB)を作成し、周辺の壁のみを抽出
    Vector3 qmin = Vector3 { std::min(start.x, start.x + delta.x) - radius, 0.0f, std::min(start.z, start.z + delta.z) - radius };
    Vector3 qmax = Vector3 { std::max(start.x, start.x + delta.x) + radius, 0.0f, std::max(start.z, start.z + delta.z) + radius };
    std::vector<const Level::AABB*> candidates;
    level.QueryWalls(qmin, qmax, candidates);

    // 衝突応答: 法線を使ったスライド処理
    // 複数の衝突面に対応するため、短い反復で残り移動を接触面に沿って投影して滑らせる
    Vector3 curStart = start;
    Vector3 curDelta = delta;
    const int kMaxSlideIter = 3; // 最大反復回数
    const float kEpsilon = 1e-4f;
    for (int iter = 0; iter < kMaxSlideIter; ++iter) {
        float bestToi = 1.0f + 1e-6f;
        Vector3 bestNormal = {0.0f, 0.0f, 0.0f};
        const Level::AABB* hitBox = nullptr;

        // 最も早く衝突する候補を探す
        for (const Level::AABB* box : candidates) {
            float toi;
            Vector3 n;
            if (GamePhysics::SweepSphereAabb2D(curStart, curDelta, radius, box->min, box->max, toi, n)) {
                if (toi >= 0.0f && toi < bestToi) {
                    bestToi = toi;
                    bestNormal = n;
                    hitBox = box;
                }
            }
        }

        if (!hitBox || bestToi > 1.0f) {
            // 衝突なし: 残り移動を全て適用して終了
            curStart.x += curDelta.x;
            curStart.z += curDelta.z;
            break;
        }

        // 衝突点まで移動
        curStart.x += curDelta.x * bestToi;
        curStart.z += curDelta.z * bestToi;

        // 小さなオフセットで法線方向に押し出してめり込みを防止
        curStart.x += bestNormal.x * kEpsilon;
        curStart.z += bestNormal.z * kEpsilon;

        // 残り移動を計算し、接触面に沿うように投影（スライド）
        Vector3 remaining = { curDelta.x * (1.0f - bestToi), 0.0f, curDelta.z * (1.0f - bestToi) };
        // 内積（X,Z 平面のみ）
        float dot = remaining.x * bestNormal.x + remaining.z * bestNormal.z;
        Vector3 slide = { remaining.x - bestNormal.x * dot, 0.0f, remaining.z - bestNormal.z * dot };

        // 速度ベクトルから法線成分を除去して摩擦を適用
        float vdot = vel_.x * bestNormal.x + vel_.z * bestNormal.z;
        vel_.x -= bestNormal.x * vdot;
        vel_.z -= bestNormal.z * vdot;
        const float kSlideFriction = 0.9f; // スライド時の減衰係数
        vel_.x *= kSlideFriction;
        vel_.z *= kSlideFriction;

        // 次の反復はスライドベクトルを移動量として扱う
        curDelta = slide;

        // 移動が微小なら終了
        if (std::sqrt(curDelta.x * curDelta.x + curDelta.z * curDelta.z) < kEpsilon)
            break;
    }

    // 反復後の位置を反映
    pos_.x = curStart.x;
    pos_.z = curStart.z;

    // 最終的に微小なめり込みが残る可能性があるため、補正を行う
    for (const Level::AABB* box : candidates) {
        GamePhysics::ResolveSphereAabb2D(pos_, radius, box->min, box->max);
    }

    // --- 5. 経路追従 (Path Following) の更新 ---
    if (!path_.empty() && pathIndex_ < (int)path_.size()) {
        Vector3 wp = path_[pathIndex_]; // 現在目指している中継点
        float dxp = wp.x - pos_.x;
        float dzp = wp.z - pos_.z;
        float distp = std::sqrt(dxp * dxp + dzp * dzp);

        // 中継点に十分近づいたら、次の地点へターゲットを切り替える
        if (distp < kWaypointThreshold) {
            pathIndex_++;
        } else {
            // 到達行動の速度を上書きし、経路に沿って移動する
            vel_.x = (dxp / distp) * moveSpeed_;
            vel_.z = (dzp / distp) * moveSpeed_;
        }
    }

    // --- 6. 接地判定 (Raycasting Down) ---
    float hitY;
    // 接地判定に使用するレイの高さはヘッダ定数を使用
    if (level.RaycastDown({ pos_.x, pos_.y + kStepHeight, pos_.z }, kStepHeight + kStepExtra, hitY)) {
        pos_.y = hitY; // 地面の高さにスナップ
    } else {
        pos_.y = 0.0f; // 下に何もなければデフォルトの高さ0へ
    }

    // 移動プラットフォーム追従: 現時点では未実装

    // --- 7. 最終的な制約と描画への反映 ---
    // ステージの境界外に出ないように制限
    // ステージ境界内に位置を制限
    level.KeepInsideBounds(pos_, kDefaultRadius);
    // その他の一般的な衝突解決
    level.ResolveCollision(pos_, kDefaultRadius);

    // 算出した座標を3Dモデルに適用
    obj_->SetTranslation(pos_);

    // カメラが存在する場合のみ描画用行列などを更新
    auto* cam = renderer_->GetDefaultCamera();
    if (cam)
        obj_->Update();

    // --- 追加: 移動方向に応じて NPC モデルの向きを更新（yaw） ---
    // 停止時には向きを変えない
    float moveDirX = vel_.x;
    float moveDirZ = vel_.z;
    if (std::sqrt(moveDirX*moveDirX + moveDirZ*moveDirZ) > kSmallEpsilon) {
        float yaw = std::atan2(moveDirX, moveDirZ); // Y 軸回転（ラジアン）
        Vector3 rot = obj_->GetRotation();
        rot.y = yaw;
        obj_->SetRotation(rot);
    }
}

void Npc::Draw()
{
    // 3Dオブジェクトの描画実行
    if (obj_)
        obj_->Draw();
}