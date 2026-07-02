#include "HomingBullet.h"
#include "Render/Object3d/Object3d.h"
#include "Math/MathUtil.h"
#include "Actor/Enemy.h"
#include "Render/Renderer/LineRenderer.h"
#include "Collision/SphereCollider.h"
#include <cmath>
#include <Windows.h>
#include "Debug/Logger.h"

#include "Collision/CollisionManager.h"

HomingBullet::HomingBullet() {}
HomingBullet::~HomingBullet() {
  if (collider_) {
    CollisionManager::GetInstance()->Remove(collider_.get());
  }
}

void HomingBullet::Initialize(Object3dRenderer* renderer, const Vector3& startPos, Enemy* target, const Vector3& initialVelocity) {
  object3d_ = std::make_unique<Object3d>();
  object3d_->Initialize(renderer);
  
  // 一旦仮のモデルとしてsuzanneを使用（スケールを小さくして色を変える）
  object3d_->SetModel("suzanne.obj"); 
  object3d_->SetScale({0.2f, 0.2f, 0.5f}); // レーザーっぽく縦長にする
  object3d_->SetColor({0.0f, 1.0f, 1.0f, 1.0f}); // シアン（水色）に光らせる
  object3d_->SetTranslation(startPos);
  
  velocity_ = initialVelocity;
  target_ = target;
  lifeTimer_ = 180; 

  // コライダーの設定
  collider_ = std::make_unique<SphereCollider>(this);
  collider_->SetRadius(0.5f);
  collider_->SetAttribute(kCollisionAttributePlayerBullet);
  collider_->SetMask(kCollisionAttributeEnemy);
  collider_->SetVelocity(velocity_);
  CollisionManager::GetInstance()->Register(collider_.get());
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
    if (target_->IsDead()) {
      target_ = nullptr; // ターゲットが死んだら誘導をやめ、そのまま直進・落下させる
    }
  }

  if (target_) {
    Vector3 currentPos = object3d_->GetTranslation();
    Vector3 targetPos = target_->GetTransform().translate;

    Vector3 toTarget = {
      targetPos.x - currentPos.x,
      targetPos.y - currentPos.y,
      targetPos.z - currentPos.z
    };

    // 発射直後（最初の15フレーム≒0.25秒）は誘導せず、重力でふんわりさせる
    if (lifeTimer_ > homingFallTime_) {
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
      homingStrength_ += homingStrengthIncrease_;
      if (homingStrength_ > homingStrengthMax_) {
        homingStrength_ = homingStrengthMax_;
      }
    }
  } else {
    // ターゲットがいない場合でも発射直後は重力をかける
    if (lifeTimer_ > homingFallTime_) {
      velocity_.y -= 0.02f;
    }
  }

  // 座標を更新
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

  // TODO: 弾の向き（回転）を進行方向（velocity_）に向ける処理を追加するとさらに綺麗になる

  object3d_->Update();
}

void HomingBullet::OnCollision(Collider* other) {
  if (isDead_) return;

  // 相手がEnemyかどうか確認
  if (other->GetAttribute() & kCollisionAttributeEnemy) {
    Enemy* enemy = dynamic_cast<Enemy*>(other->GetOwner());
    if (enemy && !enemy->IsDead()) {
      enemy->TakeDamage(damage_);
      isDead_ = true;
      Logger::Log("Homing Bullet Hit!\n");
    }
  }
}

void HomingBullet::Draw3D() {
  if (!isDead_ && object3d_) {
    object3d_->Draw();

#ifdef USE_IMGUI
    // ==== デバッグ描画 ====
    LineRenderer* lineRenderer = LineRenderer::GetInstance();
    int segments = 16;
    float angleStep = 2.0f * 3.14159265f / segments;
    Vector4 color = {0.0f, 1.0f, 1.0f, 1.0f}; 
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
