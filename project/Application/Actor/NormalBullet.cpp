#include "NormalBullet.h"
#include "Render/Object3d/Object3d.h"
#include "Math/MathUtil.h"
#include "Debug/Logger.h"
#include <Windows.h>
#include "Actor/Enemy.h"
#include "Render/Renderer/LineRenderer.h"
#include <cmath>
#include "Collision/SphereCollider.h"

#include "Collision/CollisionManager.h"

NormalBullet::NormalBullet() {}
NormalBullet::~NormalBullet() {
  if (collider_) {
    CollisionManager::GetInstance()->Remove(collider_.get());
  }
}

void NormalBullet::Initialize(Object3dRenderer* renderer, const Vector3& startPos, const Vector3& velocity) {
  object3d_ = std::make_unique<Object3d>();
  object3d_->Initialize(renderer);
  
  // 通常弾のモデル
  object3d_->SetModel("suzanne.obj"); 
  object3d_->SetScale({2.0f, 2.0f, 2.0f}); 
  object3d_->SetColor({1.0f, 0.5f, 0.0f, 1.0f}); 
  object3d_->SetTranslation(startPos);

  velocity_ = velocity; // 目標へのベクトル
  lifeTimer_ = 180; 

  // コライダーの設定
  collider_ = std::make_unique<SphereCollider>(this);
  collider_->SetRadius(2.0f);
  collider_->SetAttribute(kCollisionAttributePlayerBullet);
  collider_->SetMask(kCollisionAttributeEnemy);
  collider_->SetVelocity(velocity_);
  CollisionManager::GetInstance()->Register(collider_.get());
}

void NormalBullet::Update() {
  if (isDead_) return;

  lifeTimer_--;
  if (lifeTimer_ <= 0) {
    isDead_ = true;
  }

  // 座標を更新（ホーミングせず直進のみ）
  Vector3 pos = object3d_->GetTranslation();
  pos.x += velocity_.x;
  pos.y += velocity_.y;
  pos.z += velocity_.z;
  object3d_->SetTranslation(pos);
  // コライダーの中心座標を同期させる
  transform_.translate = pos;

  // コライダーへの速度反映
  if (collider_) {
    collider_->SetVelocity(velocity_);
  }

  object3d_->Update();
}

void NormalBullet::OnCollision(Collider* other) {
  if (isDead_) return;

  // 相手がEnemyかどうか確認
  if (other->GetAttribute() & kCollisionAttributeEnemy) {
    Enemy* enemy = dynamic_cast<Enemy*>(other->GetOwner());
    if (enemy && !enemy->IsDead()) {
      enemy->TakeDamage(damage_);
      isDead_ = true;
      Logger::Log("Normal Bullet Hit!\n");
    }
  }
}

void NormalBullet::Draw3D() {
  if (!isDead_ && object3d_) {
    object3d_->Draw();

#ifdef USE_IMGUI
    // ==== デバッグ描画 ====
    LineRenderer* lineRenderer = LineRenderer::GetInstance();
    int segments = 16;
    float angleStep = 2.0f * 3.14159265f / segments;
    Vector4 color = {0.0f, 0.0f, 1.0f, 1.0f}; 
    float radius = 0.5f;
    Vector3 pos = object3d_->GetTranslation();

    for (int i = 0; i < segments; ++i) {
      float angle1 = i * angleStep;
      float angle2 = (i + 1) * angleStep;

      // XY plane
      Vector3 p1_xy = {pos.x + std::cos(angle1) * radius, pos.y + std::sin(angle1) * radius, pos.z};
      Vector3 p2_xy = {pos.x + std::cos(angle2) * radius, pos.y + std::sin(angle2) * radius, pos.z};
      lineRenderer->DrawLine(p1_xy, p2_xy, color);

      // XZ plane
      Vector3 p1_xz = {pos.x + std::cos(angle1) * radius, pos.y, pos.z + std::sin(angle1) * radius};
      Vector3 p2_xz = {pos.x + std::cos(angle2) * radius, pos.y, pos.z + std::sin(angle2) * radius};
      lineRenderer->DrawLine(p1_xz, p2_xz, color);

      // YZ plane
      Vector3 p1_yz = {pos.x, pos.y + std::cos(angle1) * radius, pos.z + std::sin(angle1) * radius};
      Vector3 p2_yz = {pos.x, pos.y + std::cos(angle2) * radius, pos.z + std::sin(angle2) * radius};
      lineRenderer->DrawLine(p1_yz, p2_yz, color);
    }
#endif
  }
}
