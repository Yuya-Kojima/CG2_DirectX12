#include "EnemyBullet.h"
#include "Render/Object3d/Object3d.h"
#include "Math/MathUtil.h"
#include "Actor/Player.h"
#include <cmath>

EnemyBullet::EnemyBullet() {}
EnemyBullet::~EnemyBullet() {}

void EnemyBullet::Initialize(Object3dRenderer* renderer, const Vector3& startPos, const Vector3& velocity, Player* player) {
  object3d_ = std::make_unique<Object3d>();
  object3d_->Initialize(renderer);
  
  // 見た目の設定 (敵の弾は赤系にする)
  object3d_->SetModel("suzanne.obj"); 
  object3d_->SetScale({1.5f, 1.5f, 1.5f}); // 少し小さめ
  object3d_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤色
  object3d_->SetTranslation(startPos);

  velocity_ = velocity; 
  player_ = player;
  lifeTimer_ = 180; // 約3秒で消滅
}

void EnemyBullet::Update() {
  if (isDead_) return;

  lifeTimer_--;
  if (lifeTimer_ <= 0) {
    isDead_ = true;
  }

  // 位置の更新
  Vector3 pos = object3d_->GetTranslation();
  pos.x += velocity_.x;
  pos.y += velocity_.y;
  pos.z += velocity_.z;
  object3d_->SetTranslation(pos);

  // プレイヤーへの当たり判定
  if (player_ && !player_->IsDead()) {
    Vector3 playerPos = player_->GetTransform().translate;
    
    // 線分と点の距離計算（NormalBulletと同じロジック）
    Vector3 prevPos = {pos.x - velocity_.x, pos.y - velocity_.y, pos.z - velocity_.z};
    Vector3 lineDir = velocity_;
    float lineLen = Length(lineDir);
    if (lineLen > 0.001f) {
      lineDir.x /= lineLen; lineDir.y /= lineLen; lineDir.z /= lineLen;
    }
    
    Vector3 toPlayer = {playerPos.x - prevPos.x, playerPos.y - prevPos.y, playerPos.z - prevPos.z};
    float t = Dot(toPlayer, lineDir);
    t = (std::max)(0.0f, (std::min)(lineLen, t)); 
    
    Vector3 closestPoint = {prevPos.x + lineDir.x * t, prevPos.y + lineDir.y * t, prevPos.z + lineDir.z * t};
    Vector3 diff = {playerPos.x - closestPoint.x, playerPos.y - closestPoint.y, playerPos.z - closestPoint.z};
    
    float dist = Length(diff);

    // 弾の半径
    float bulletRadius = 1.5f;
    
    // プレイヤーの半径 (スケールを考慮)
    Vector3 playerScale = player_->GetTransform().scale;
    float maxScale = (std::max)(playerScale.x, (std::max)(playerScale.y, playerScale.z));
    float playerRadius = 1.5f * maxScale; // プレイヤーの基本半径を1.5とする
    
    // 衝突判定
    if (dist < bulletRadius + playerRadius) {
      isDead_ = true; 
      player_->TakeDamage(1); // プレイヤーに1ダメージ
    }
  }

  object3d_->Update();
}

void EnemyBullet::Draw3D() {
  if (!isDead_ && object3d_) {
    object3d_->Draw();
  }
}
