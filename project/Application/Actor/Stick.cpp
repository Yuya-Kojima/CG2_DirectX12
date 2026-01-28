#include "Actor/Stick.h"
#include "Actor/Level.h"
#include "Camera/GameCamera.h"
#include "Model/ModelManager.h"
#include "Model/Model.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
#include "Debug/Logger.h"
#include <algorithm>
#include <cmath>

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
    // デフォルト：地面に横たわる向き
    obj_->SetRotation(dropRotation_);
    obj_->SetTranslation(pos_);
    try {
        Logger::Log(std::format("[スティック] 初期化: pos=({:.2f},{:.2f},{:.2f})\n", pos_.x, pos_.y, pos_.z));
    } catch (...) {
        Logger::Log(std::string("[スティック] 初期化: pos=(") + std::to_string(pos_.x) + "," + std::to_string(pos_.y) + "," + std::to_string(pos_.z) + ")\n");
    }

}

void Stick::SetId(uint32_t id)
{
    id_ = id;
    // もしこのスティックが既にレベルに所属しており保持されていない場合、
    // 当たり判定用の OBB を登録して衝突を有効にします。
    if (level_ && !held_) {
        // Avoid duplicate registration
        if (registeredOwnerId_ == 0) {
            // DropExternal と同じ半幅計算を使用する
            Vector3 baseHalfExtents = { 0.75f, 0.5f, 4.0f };
            Vector3 s = obj_ ? obj_->GetScale() : Vector3 { 1.0f, 1.0f, 1.0f };
            Vector3 actualHalfExtents = {
                baseHalfExtents.x * s.x,
                baseHalfExtents.y * s.y,
                baseHalfExtents.z * s.z
            };

            Level::OBB obb;
            obb.center = pos_;
            obb.halfExtents = actualHalfExtents;
            obb.yaw = obj_ ? obj_->GetRotation().y : rotation_.y;
            obb.ownerLayer = layer_;
            obb.ownerId = id_;

            level_->ClearOwnerId(id_);
            level_->AddWallOBB(obb, false);
            registeredOwnerId_ = id_;
            level_->RebuildWallGrid();
            collisionEnabled_ = true;
        }
    }
}


void Stick::SetLevel(Level* lvl)
{
    level_ = lvl;
    // 床の上に合わせる: 現在位置の少し上から下方向へレイを飛ばして着地位置を求める
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

    // 1. 回転の更新
    // - 持っている間はプレイヤーの入力で瞬時に角度を変えられる想定なので
    //   補間せず heldRotation_ を直接反映する（遅延による違和感を防ぐ）。
    // - 地面に置かれている状態では、落ち着いた見た目のために補間で回転を戻す。
    if (held_) {
        rotation_ = heldRotation_;
    } else {
        // 2. 回転の線形補間（Lerp）処理
        // 持ち替えた時にパッと切り替わるのではなく、スムーズに回転させます
        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
        float lerpT = std::min(1.0f, rotationLerpSpeed_ * dt);

        rotation_.x = lerp(rotation_.x, dropRotation_.x, lerpT);
        rotation_.y = lerp(rotation_.y, dropRotation_.y, lerpT);
        rotation_.z = lerp(rotation_.z, dropRotation_.z, lerpT);
    }

    // モデルに回転を適用
    if (obj_)
        obj_->SetRotation(rotation_);

    // 3. カメラが存在する場合のみ描画行列を更新
    auto* cam = renderer_->GetDefaultCamera();
    if (cam)
        obj_->Update();

    // このスティックがレベルに OBB を登録済みで、かつ設置状態（誰も持っていない）なら
    // レベル側の OBB を現在の位置・回転に同期させる
    if (level_ && registeredOwnerId_ != 0 && !held_) {
        // モデルのスケールから半幅を再計算する（DropExternal と同様のロジック）
        Vector3 baseHalfExtents = { 0.75f, 0.5f, 4.0f };
        Vector3 s = obj_ ? obj_->GetScale() : Vector3 { 1.0f, 1.0f, 1.0f };
        Vector3 actualHalfExtents = {
            baseHalfExtents.x * s.x,
            baseHalfExtents.y * s.y,
            baseHalfExtents.z * s.z
        };

        Level::OBB obb;
        obb.center = pos_;
        obb.halfExtents = actualHalfExtents;
        obb.yaw = rotation_.y;
        obb.ownerId = registeredOwnerId_;
        obb.ownerLayer = layer_;

        level_->UpdateOBB(registeredOwnerId_, obb);
    }
}

void Stick::Draw()
{
    if (obj_)
        obj_->Draw();
}

    // 外部から拾う／落とすが呼ばれた際の委譲処理
void Stick::PickUp() { PickUpExternal(); }
void Stick::Drop() { DropExternal(); }

void Stick::PickUpExternal()
{
    // 拾い上げた時の処理
    held_ = true;
    collisionEnabled_ = false; // 持っている間は物理判定を無効化

    // 拾った時はプレイヤーの横や頭上に置かれる想定（呼び出し側に依存）。
    // 体と干渉しないよう少し持ち上げる。
    pos_.y += 0.25f;
    if (obj_) {
    // 拾ったときは現在の横たわる角度を保持する
        heldRotation_ = rotation_;
        obj_->SetTranslation(pos_);
        // Keep the model rotation as it was on ground
        obj_->SetRotation(heldRotation_);
    }

    // 地面に置いた際にレベルに登録していた当たり判定(OBB)を削除
    if (level_ && registeredOwnerId_ != 0) {
        level_->RemoveOBBsByOwnerId(registeredOwnerId_);
        registeredOwnerId_ = 0;
    }
}

void Stick::AdjustHeldYaw(float delta)
{
    // 持っている間にヨーを変える。15度刻みにスナップする
    const float k15deg = 3.14159265f * (15.0f / 180.0f);
    heldRotation_.y += delta;
    // snap to nearest 15 degrees
    float n = std::round(heldRotation_.y / k15deg);
    heldRotation_.y = n * k15deg;
    rotation_.y = heldRotation_.y;
    if (obj_) obj_->SetRotation(rotation_);
}

void Stick::SetHoldSideFromPlayerPos(const Vector3& playerPos)
{
    // スティックがプレイヤーの左か右かを X軸の差分で判定する
    float dx = pos_.x - playerPos.x;
    holdSideX = (dx < 0.0f) ? -1 : 1;
}

void Stick::DropExternal()
{
    // 所持状態を終了して落とす：保持中の回転を使って配置する
    held_ = false;
    collisionEnabled_ = false;
    // 投げた直後に自分が当たらないようにタイマーを設定
    collisionDisableTimer_ = collisionDisableDuration_;

    if (level_) {
        // --- 1. モデルのサイズ定義（stick.obj の頂点データに基づく） ---
        // stick.obj は x:±0.75, y:±0.5, z:±4.0 なので、以下の値が「中心からの距離」になります
        Vector3 baseHalfExtents = { 0.75f, 0.5f, 4.0f };

        // 現在のスケールを取得（Initializeで 0.5f が設定されている想定）
        Vector3 s = obj_ ? obj_->GetScale() : Vector3 { 1.0f, 1.0f, 1.0f };

        // 実際のワールド空間での「半分の厚み・高さ・長さ」
        Vector3 actualHalfExtents = {
            baseHalfExtents.x * s.x,
            baseHalfExtents.y * s.y,
            baseHalfExtents.z * s.z
        };

        // --- 2. 座標の調整（接地処理） ---
        // 可能であれば現在の XZ の下の地面にスナップして、
        // メッシュの底面が地面に接するように配置する: center.y = groundY + halfHeight
        float hitY = 0.0f;
        const float kSink = 0.5f; // small downward adjustment to remove visible gap
        if (level_ && level_->RaycastDown({ pos_.x, pos_.y + 1.0f, pos_.z }, 2.0f, hitY)) {
            pos_.y = hitY + actualHalfExtents.y - kSink;
        } else {
            // fallback: assume ground at y=0
            pos_.y = std::max(0.0f, actualHalfExtents.y - kSink);
        }

        // モデルのルート行列にローカルオフセットが入っている場合に対応する。
        // ルート行列の Y オフセット値をスケールと共に参照し、
        // メッシュの底面が地面に合うように補正する。
        if (obj_) {
            Model *m = obj_->GetModel();
            if (m != nullptr) {
                const Matrix4x4 &root = m->GetRootLocalMatrix();
                Vector3 s = obj_->GetScale();
                // root.m[3][1] is the Y translation in the model's local matrix
                pos_.y -= root.m[3][1] * s.y;
            }
        }

        // --- 3. OBB の設定 ---
        Level::OBB obb;

        // 当たり判定の箱の中心をモデルの配置座標と一致させる
        obb.center = pos_;

        // 当たり判定の大きさを、スケーリング後のモデルサイズと一致させる
        obb.halfExtents = actualHalfExtents;

        // モデルの回転は保持中に調整した heldRotation_ を優先して使う
        // これにより、持ち上げた状態で回転させた角度でそのまま置ける
        rotation_ = heldRotation_;
        if (obj_) {
            obj_->SetRotation(rotation_);
        }
        // Ensure the resting (drop) target matches the rotation we are placing
        dropRotation_ = rotation_;
        obb.yaw = rotation_.y;

        // 識別情報の登録
        obb.ownerLayer = layer_;
        obb.ownerId = id_;

        // --- 4. レベルへの登録 ---
        // モデルの最終行列を更新してから OBB を登録する（視覚位置と当たり判定のズレを防ぐ）
        if (obj_) {
            obj_->SetTranslation(pos_);
            obj_->SetRotation(rotation_);
            obj_->Update();
        }

        // 衝突判定が有効になるまで OBB の登録を遅延させる。
        // これはプレイヤーがまだ重なっている状態で OBB を置くと
        // 即時めり込みやスタックすることがあるための対策。
        pendingRegister_ = true;
        pendingHalfExtents_ = actualHalfExtents;
        pendingYaw_ = rotation_.y;
    }

    // モデルの位置は既に更新済み上記で更新済み
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

            // Now register pending OBB if any
            if (pendingRegister_ && level_) {
                Level::OBB obb;
                obb.center = pos_;
                obb.halfExtents = pendingHalfExtents_;
                obb.yaw = pendingYaw_;
                obb.ownerLayer = layer_;
                obb.ownerId = id_;

                level_->ClearOwnerId(id_);
                level_->AddWallOBB(obb, false);
                registeredOwnerId_ = id_;
                level_->RebuildWallGrid();
                pendingRegister_ = false;
            }
        }
    }
}

bool Stick::IsCollisionEnabled() const
{
    return collisionEnabled_;
}