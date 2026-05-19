#include "HomingBullet.h"
#include "Render/Object3d/Object3d.h"
#include "Math/MathUtil.h"
#include "Actor/Enemy.h"

HomingBullet::HomingBullet() {}
HomingBullet::~HomingBullet() {}

void HomingBullet::Initialize(Object3dRenderer* renderer, const Vector3& startPos, Enemy* target, const Vector3& initialVelocity) {
  object3d_ = std::make_unique<Object3d>();
  object3d_->Initialize(renderer);
  
  // 一旦仮のモデルとしてsuzanneを使用（スケールを小さくして色を変える）
  object3d_->SetModel("suzanne.obj"); 
  object3d_->SetScale({0.2f, 0.2f, 0.5f}); // レーザーっぽく縦長にする
  object3d_->SetColor({0.0f, 1.0f, 1.0f, 1.0f}); // シアン（水色）に光らせる
  object3d_->SetTranslation(startPos);

  target_ = target;
  velocity_ = initialVelocity; // 発射時は指定したベクトルへ打ち上がる
}

void HomingBullet::Update() {
  if (isDead_) return;

  lifeTimer_--;
  if (lifeTimer_ <= 0) {
    isDead_ = true;
    return;
  }

  // ターゲットが生きていれば誘導ベクトルを計算
  if (target_) {
    Vector3 currentPos = object3d_->GetTranslation();
    Vector3 targetPos = target_->GetTransform().translate;

    Vector3 toTarget = {
      targetPos.x - currentPos.x,
      targetPos.y - currentPos.y,
      targetPos.z - currentPos.z
    };
    float dist = Length(toTarget);

    // 当たり判定（簡易的に距離が近づいたらヒットとする）
    if (dist < 3.0f) { 
      isDead_ = true; // 着弾して消滅
      
      // 敵を撃破（Destroy関数を呼ぶ）
      target_->Destroy();
      
      #include <Windows.h>
      OutputDebugStringA("Enemy Destroyed!\n");
      
      return;
    }

    // 発射直後（最初の15フレーム≒0.25秒）は誘導せず、重力でふんわりさせる
    if (lifeTimer_ > 165) {
      velocity_.y -= 0.02f; // 軽い重力
    } else {
      // 頂点に達したあたりから、一気に敵に向かって誘導を開始する
      Vector3 normToTarget = Normalize(toTarget);
      Vector3 desiredVelocity = {
        normToTarget.x * speed_,
        normToTarget.y * speed_,
        normToTarget.z * speed_
      };
      
      velocity_ = Lerp(velocity_, desiredVelocity, homingStrength_);
      
      // かなり強めに誘導をかけることで、通り過ぎずに突き刺さる
      homingStrength_ += 0.015f;
      if (homingStrength_ > 0.25f) {
        homingStrength_ = 0.25f;
      }
    }
  }

  // 座標を更新
  Vector3 pos = object3d_->GetTranslation();
  pos.x += velocity_.x;
  pos.y += velocity_.y;
  pos.z += velocity_.z;
  object3d_->SetTranslation(pos);

  // TODO: 弾の向き（回転）を進行方向（velocity_）に向ける処理を追加するとさらに綺麗になる

  object3d_->Update();
}

void HomingBullet::Draw3D() {
  if (!isDead_ && object3d_) {
    object3d_->Draw();
  }
}
