#include "Actor/Stick.h"
#include "Actor/Level.h"
#include "Camera/GameCamera.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
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
    std::string m = ModelManager::ResolveModelPath("stick.obj");
    ModelManager::GetInstance()->LoadModel(m);
    obj_->SetModel(m);
    obj_->SetScale({ 0.5f, 0.5f, 0.5f });
    obj_->SetTranslation(pos_);
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

    // めり込み防止のため、少しだけ上方にオフセット
    pos_.y += 0.05f;
    if (obj_)
        obj_->SetTranslation(pos_);

    // 地面に置いた際に登録した「壁としての当たり判定」をレベルから削除
    if (level_ && registeredOwnerId_ != 0) {
        level_->RemoveWallsByOwnerId(registeredOwnerId_);
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
        Vector3 min = { pos_.x - 0.3f, pos_.y - 0.3f, pos_.z - 0.3f };
        Vector3 max = { pos_.x + 0.3f, pos_.y + 0.3f, pos_.z + 0.3f };

        // レベルに壁データを追加し、このスティックのIDをオーナーとして紐付け
        level_->AddWallAABB(min, max, false, layer_, id_);
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