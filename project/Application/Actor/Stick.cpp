#include "Actor/Stick.h"
#include "Actor/Level.h"
#include "Camera/GameCamera.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
#include "Debug/Logger.h"
#include <algorithm>

// Windows等の環境で衝突する可能性のあるマクロを解除
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

void Stick::Initialize(Object3dRenderer* renderer, const Vector3& pos)
{
    renderer_ = renderer;
    pos_ = pos;

    // オブジェクトの生成
    obj_ = std::make_unique<Object3d>();
    obj_->Initialize(renderer_);

    // スティック用モデルの読み込みと初期設定
    ModelManager::GetInstance()->LoadModel("stick.obj");
    obj_->SetModel("stick.obj");
    obj_->SetScale({ 0.5f, 0.5f, 0.5f });
    // Default: lying on the ground horizontally
    obj_->SetRotation(dropRotation_);
    obj_->SetTranslation(pos_);
    try {
        Logger::Log(std::format("[Stick] Initialize: pos=({:.2f},{:.2f},{:.2f})\n", pos_.x, pos_.y, pos_.z));
    } catch (...) {
        Logger::Log(std::string("[Stick] Initialize: pos=(") + std::to_string(pos_.x) + "," + std::to_string(pos_.y) + "," + std::to_string(pos_.z) + ")\n");
    }
}

void Stick::SetLevel(Level* lvl)
{
    level_ = lvl;
    // Align stick to be on top of the floor: Raycast down from a little above current pos
    if (level_) {
        float hitY;
        if (level_->RaycastDown({ pos_.x, pos_.y + 1.0f, pos_.z }, 2.0f, hitY)) {
            pos_.y = hitY + 0.1f; // slightly above floor
        } else {
            // fallback to default ground
            pos_.y = 0.1f;
        }
        if (obj_)
            obj_->SetTranslation(pos_);
    }
}

void Stick::Update(float dt)
{
    if (!obj_ || !renderer_)
        return;

    // 1. 状態に応じた目標回転角の決定
    Vector3 targetRot = held_ ? heldRotation_ : dropRotation_;

    // 2. 回転の線形補間（Lerp）処理
    // 持ち替えた時にパッと切り替わるのではなく、スムーズに回転させます
    auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    float lerpT = std::min(1.0f, rotationLerpSpeed_ * dt);

    rotation_.x = lerp(rotation_.x, targetRot.x, lerpT);
    rotation_.y = lerp(rotation_.y, targetRot.y, lerpT);
    rotation_.z = lerp(rotation_.z, targetRot.z, lerpT);

    // 回転をモデルに適用
    if (obj_)
        obj_->SetRotation(rotation_);

    // 3. カメラが存在する場合のみ描画行列を更新
    auto* cam = renderer_->GetDefaultCamera();
    if (cam)
        obj_->Update();
}

void Stick::Draw()
{
    if (obj_)
        obj_->Draw();
}

// 外部から拾う/落とすと呼ばれた際の委譲処理
void Stick::PickUp() { PickUpExternal(); }
void Stick::Drop() { DropExternal(); }

void Stick::PickUpExternal()
{
    // 拾い上げた時の処理
    held_ = true;
    collisionEnabled_ = false; // 持っている間は物理判定を無効化

    // When picked up, position at player's side / above head depending on caller
    // Slightly lift so it doesn't intersect body
    pos_.y += 0.25f;
    if (obj_) {
        obj_->SetTranslation(pos_);
        // Rotate to be horizontal above/alongside player
        obj_->SetRotation(heldRotation_);
    }

    // 地面に置いた際に登録した「壁としての当たり判定」をレベルから削除
    if (level_ && registeredOwnerId_ != 0) {
        level_->RemoveOBBsByOwnerId(registeredOwnerId_);
        registeredOwnerId_ = 0;
    }
}

void Stick::DropExternal()
{
    // ドロップ時の処理
    held_ = false;
    collisionEnabled_ = false; // 離した瞬間に自分の体にぶつからないよう一時無効化

    // 衝突無効化タイマーを開始
    collisionDisableTimer_ = collisionDisableDuration_;

    // 地面に置かれたアイテムを「障害物」としてレベルに登録
    // これにより、他のNPCなどがこのアイテムを避けて通るようになります
    if (level_) {
        // スティックの周囲に小さな衝突判定領域(AABB)を作成
        // Ensure the stick lies flat on the ground when dropped
        pos_.y -= 0.15f; // lower slightly to contact ground
        if (obj_) obj_->SetRotation(dropRotation_);

        // OBB として登録する: スティックはXZ平面回転のみ考慮
        Level::OBB obb;
        obb.center = pos_;
        // スティックは細長い形状なので、scaleから半幅を推定
        obb.halfExtents = Vector3 { 0.5f * 0.5f, 0.1f, 0.2f };
        // Y回転は rotation_.y を使用（ラジアン想定）
        obb.yaw = rotation_.y;
        obb.ownerLayer = layer_;
        obb.ownerId = id_;

        level_->AddWallOBB(obb, false);
        registeredOwnerId_ = id_;

        // グリッドを再構築してAIが認識できるようにする
        level_->RebuildWallGrid();
    }
}

void Stick::UpdateCollisionTimer(float dt)
{
    // 衝突判定が無効化されている間、タイマーを進める
    if (!collisionEnabled_ && collisionDisableTimer_ > 0.0f) {
        collisionDisableTimer_ -= dt;

        // 指定時間が経過したら衝突判定を有効に戻す
        if (collisionDisableTimer_ <= 0.0f) {
            collisionDisableTimer_ = 0.0f;
            collisionEnabled_ = true;
        }
    }
}

bool Stick::IsCollisionEnabled() const
{
    return collisionEnabled_;
}