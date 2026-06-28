#include "Actor/Enemy.h"
#include "Camera/ICamera.h"
#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Math/MathUtil.h"
#include "Render/Object3d/Object3d.h"
#include "Render/Particle/IParticleEmitter.h"
#include "Behavior/IEnemyBehavior.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleManager.h"
#include "Actor/Player.h" 
#include <Windows.h> // OutputDebugStringA用
#include <cmath>

Enemy::Enemy() = default;

Enemy::~Enemy() {
  if (collider_) {
    CollisionManager::GetInstance()->Remove(collider_.get());
  }
}

void Enemy::Initialize() {
  // 敵のコライダーの初期化
  collider_ = std::make_unique<SphereCollider>(this);
  collider_->SetRadius(0.8f); // 敵の当たり判定の大きさをモデルより一回り小さめに設定
  collider_->SetAttribute(kCollisionAttributeEnemy); // 自分は「敵」
  collider_->SetMask(kCollisionAttributePlayer |
                     kCollisionAttributePlayerBullet); // 自機や自機の弾と当たる
  CollisionManager::GetInstance()->Register(collider_.get());
}

void Enemy::Update() {
  // 死んでいる場合は当たり判定を消して何もさせない
  if (isDead_) {
    if (collider_) {
      CollisionManager::GetInstance()->Remove(collider_.get());
      collider_.reset(); // コライダーを破棄
    }
    return;
  }

  aliveTime_ += 1.0f / 60.0f; // 簡易的に60FPS固定で時間計算

  if (behavior_) {
    behavior_->Update(this);
  }

  // 被弾時の点滅処理（赤色にする）
  if (hitFlashTimer_ > 0) {
    hitFlashTimer_--;
    if (model_) {
      model_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤色
    }
  } else {
    if (model_) {
      model_->SetColor(baseColor_); // 元の色
    }
  }

  UpdateTransform();
}

void Enemy::UpdateTransform() {
  // モデルが存在していれば、敵の座標をモデルに反映して更新
  if (model_) {
    model_->SetTranslation(transform_.translate);
    model_->SetRotation(transform_.rotate);
    model_->SetScale(transform_.scale);
    model_->Update();
  }
}

void Enemy::Draw3D() {
  // 3Dモデルの描画
  if (model_) {
    model_->Draw();
  }
}

void Enemy::OnCollision(Collider *other) {
  // プレイヤーと衝突した場合、自身もダメージを受けて自爆する
  if (other->GetOwner() && dynamic_cast<Player*>(other->GetOwner())) {
    OutputDebugStringA("Enemy Self-Destruct into Player!\n");
    TakeDamage(999, true); // true を渡して自爆であることを知らせる
  }
}

void Enemy::TakeDamage(int damage, bool isSelfDestruct) {
  if (isDead_) {
    return;
  }

  hp_ -= damage;
  hitFlashTimer_ = 5; // 5フレーム間点滅

  if (hp_ <= 0) {
    OutputDebugStringA("Enemy Destroyed!\n");
    
    if (onDestroyedCallback_) {
      onDestroyedCallback_(isSelfDestruct);
    }
    Destroy();
  }
}
