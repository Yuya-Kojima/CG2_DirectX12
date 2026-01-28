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
    std::string m = ModelManager::ResolveModelPath("npc.obj");
    ModelManager::GetInstance()->LoadModel(m);
    obj_->SetModel(m);

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

    // --- 2. 経路の再計画 (Path Re-planning) ---
    replanTimer_ -= dt;
    // 計算負荷を抑えるため、一定間隔（0.5秒）ごとに実行
    if (nav_ && replanTimer_ <= 0.0f) {
        int sx, sz, gx, gz;
        // 現在地と目的地をグリッド座標に変換
        if (nav_->WorldToTile(pos_, sx, sz) && nav_->WorldToTile(targetPos, gx, gz)) {
            // A*などのアルゴリズムで経路リストを生成
            path_ = nav_->FindPath(sx, sz, gx, gz);
            pathIndex_ = 0;
        }
        replanTimer_ = 0.5f; // タイマーリセット
    }

    // --- 3. ハザード（危険地帯）の早期検知と回避 ---
    // 次のフレームで移動する予定の座標を計算
    Vector3 next { pos_.x + vel_.x * dt, 0.0f, pos_.z + vel_.z * dt };
    if (level.IsHazardHit(next, 0.3f)) {
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
    const float radius = 0.3f; // NPCの当たり判定半径
    Vector3 start = pos_;
    Vector3 delta { vel_.x * dt, 0.0f, vel_.z * dt };

    // 移動経路を包含する矩形領域(AABB)を作成し、周辺の壁のみを抽出
    Vector3 qmin = Vector3 { std::min(start.x, start.x + delta.x) - radius, 0.0f, std::min(start.z, start.z + delta.z) - radius };
    Vector3 qmax = Vector3 { std::max(start.x, start.x + delta.x) + radius, 0.0f, std::max(start.z, start.z + delta.z) + radius };
    std::vector<const Level::AABB*> candidates;
    level.QueryWalls(qmin, qmax, candidates);

    // 衝突までの時間（TOI: Time of Impact）を求めて、壁を突き抜けない位置まで移動を制限
    float tMove = 1.0f;
    for (const Level::AABB* box : candidates) {
        float toi;
        Vector3 n;
        // スフィア（NPC）対 AABB（壁）のスイープテスト
        if (GamePhysics::SweepSphereAabb2D(start, delta, radius, box->min, box->max, toi, n)) {
            if (toi < tMove)
                tMove = toi;
        }
    }
    // 実際に衝突しない安全な距離まで移動させる
    start.x += delta.x * tMove;
    start.z += delta.z * tMove;

    // 浮動小数点の誤差による壁へのめり込みを完全に解決（押し出し処理）
    for (const Level::AABB* box : candidates) {
        GamePhysics::ResolveSphereAabb2D(start, radius, box->min, box->max);
    }
    pos_.x = start.x;
    pos_.z = start.z;

    // --- 5. 経路追従 (Path Following) の更新 ---
    if (!path_.empty() && pathIndex_ < (int)path_.size()) {
        Vector3 wp = path_[pathIndex_]; // 現在目指している中継点
        float dxp = wp.x - pos_.x;
        float dzp = wp.z - pos_.z;
        float distp = std::sqrt(dxp * dxp + dzp * dzp);

        // 中継点に十分近づいたら、次の地点へターゲットを切り替える
        if (distp < 0.3f) {
            pathIndex_++;
        } else {
            // 到達行動の速度を上書きし、経路に沿って移動する
            vel_.x = (dxp / distp) * moveSpeed_;
            vel_.z = (dzp / distp) * moveSpeed_;
        }
    }

    // --- 6. 接地判定 (Raycasting Down) ---
    float hitY;
    const float stepHeight = 0.3f; // 少し高い段差でも乗り越えられるよう上からレイを飛ばす
    // 足元に下向きのレイを飛ばし、床やプラットフォームとの交点を取得
    if (level.RaycastDown({ pos_.x, pos_.y + stepHeight, pos_.z }, stepHeight + 0.2f, hitY)) {
        pos_.y = hitY; // 地面の高さにスナップ
    } else {
        pos_.y = 0.0f; // 下に何もなければデフォルトの高さ0へ
    }

    // 移動プラットフォーム追従: 現時点では未実装

    // --- 7. 最終的な制約と描画への反映 ---
    // ステージの境界外に出ないように制限
    level.KeepInsideBounds(pos_, 0.3f);
    // その他の一般的な衝突解決
    level.ResolveCollision(pos_, 0.3f);

    // 算出した座標を3Dモデルに適用
    obj_->SetTranslation(pos_);

    // カメラが存在する場合のみ描画用行列などを更新
    auto* cam = renderer_->GetDefaultCamera();
    if (cam)
        obj_->Update();
}

void Npc::Draw()
{
    // 3Dオブジェクトの描画実行
    if (obj_)
        obj_->Draw();
}