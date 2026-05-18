#include "NormalBullet.h"
#include "Render/Object3d/Object3d.h"
#include "Math/MathUtil.h"
#include <Windows.h>

NormalBullet::NormalBullet() {}
NormalBullet::~NormalBullet() {}

void NormalBullet::Initialize(Object3dRenderer* renderer, const Vector3& startPos, const Vector3& velocity, const std::vector<Object3d*>& enemies) {
  object3d_ = std::make_unique<Object3d>();
  object3d_->Initialize(renderer);
  
  // 通常弾のモデル（仮で細長いスザンヌ）
  object3d_->SetModel("suzanne.obj"); 
  // 視認性を上げるためにサイズを大きくする
  object3d_->SetScale({0.4f, 0.4f, 1.0f}); 
  object3d_->SetColor({1.0f, 0.5f, 0.0f, 1.0f}); // 通常弾はオレンジ色にする
  object3d_->SetTranslation(startPos);

  velocity_ = velocity; // まっすぐ飛ぶベクトル
  enemies_ = enemies;
}

void NormalBullet::Update() {
  if (isDead_) return;

  lifeTimer_--;
  if (lifeTimer_ <= 0) {
    isDead_ = true;
    return;
  }

  // 座標を更新（ホーミングせず直進のみ）
  Vector3 pos = object3d_->GetTranslation();
  pos.x += velocity_.x;
  pos.y += velocity_.y;
  pos.z += velocity_.z;
  object3d_->SetTranslation(pos);

  // 敵への当たり判定（簡易的に距離で判定）
  for (Object3d* enemy : enemies_) {
    if (!enemy) continue;
    if (enemy->GetScale().x <= 0.0f) continue; // 既に死んでいる敵は無視

    Vector3 enemyPos = enemy->GetTranslation();
    Vector3 diff = {
      enemyPos.x - pos.x,
      enemyPos.y - pos.y,
      enemyPos.z - pos.z
    };
    float dist = Length(diff);

    if (dist < 3.0f) {
      isDead_ = true; // 弾が消える
      enemy->SetScale({0.0f, 0.0f, 0.0f}); // 敵を倒す（スケール0）
      OutputDebugStringA("Normal Bullet Hit!\n");
      break;
    }
  }

  object3d_->Update();
}

void NormalBullet::Draw3D() {
  if (!isDead_ && object3d_) {
    object3d_->Draw();
  }
}
