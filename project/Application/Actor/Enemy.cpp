#include "Actor/Enemy.h"
#include "Camera/ICamera.h"
#include "Camera/RailCamera.h"
#include "Effect/EffectManager.h"
#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Math/MathUtil.h"
#include "Render/Object3d/Object3d.h"
#include "Render/Particle/IParticleEmitter.h"
#include "Behavior/IEnemyBehavior.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleManager.h"
#include "Actor/Player.h" 
#include "Debug/Logger.h"
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
  collider_->SetRadius(0.8f); // 敵の当たり判定の大きさをモデルより少し小さめに設定
  collider_->SetAttribute(kCollisionAttributeEnemy); // 自機から見て「敵」
  collider_->SetMask(kCollisionAttributePlayer |
                     kCollisionAttributePlayerBullet); // 自機や自機の弾と当たる
  CollisionManager::GetInstance()->Register(collider_.get());
}

void Enemy::Update() {
  // 死んでいる場合は当たり判定を消して何もさせない
  if (isDead_) {
    if (collider_) {
      collider_->SetEnable(false); // コライダーを無効化（メモリは破棄しない）
    }
    return;
  }

  // --- カメラ基準値の毎フレーム更新 ---
  if (camera_) {
    if (auto railCam = dynamic_cast<const RailCamera*>(camera_)) {
      basePos_ = railCam->GetRailPosition();
      baseForward_ = railCam->GetRailForward();
      baseRight_ = railCam->GetRailRight();
      baseUp_ = railCam->GetRailUp();
    } else {
      basePos_ = camera_->GetTranslate();
      baseForward_ = camera_->GetForward();
      baseRight_ = camera_->GetRight();
      baseUp_ = camera_->GetUp();
    }
  }

  aliveTime_ += 1.0f / 60.0f; // 簡易的に60FPS固定で時間計算

  if (behavior_) {
    behavior_->Update(this);
  }

  // 被弾時は赤色にする
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
    // モデルの更新
    UpdateTransform();

    // 連続衝突判定用に速度を計算してコライダーに渡す
    if (collider_) {
      Vector3 velocity = {
          transform_.translate.x - previousPos_.x,
          transform_.translate.y - previousPos_.y,
          transform_.translate.z - previousPos_.z
      };
      collider_->SetVelocity(velocity);
    }
    previousPos_ = transform_.translate;
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
    Player* p = dynamic_cast<Player*>(other->GetOwner());
    if (p->GetInvincibleTimer() > 0) {
      return; // 無敵中なら食らわない
    }
    p->TakeDamage(1); // プレイヤーにダメージを与える
    Logger::Log("Enemy Self-Destruct into Player!\n");
    TakeDamage(999, true); // true を渡して自爆であることを知らせる
  }
}

void Enemy::TakeDamage(int damage, bool isSelfDestruct) {
  if (isDead_) {
    return;
  }

  hp_ -= damage;
  hitFlashTimer_ = 5; // 5フレーム点滅

  if (hp_ <= 0) {
    Logger::Log("Enemy Destroyed!\n");
    
    // コールバック（ボス等の特別処理）が設定されていない、かつ自爆でない場合、自律的に爆発する
    if (!onDestroyedCallback_ && !isSelfDestruct) {
      EffectManager::GetInstance()->PlayEnemyDeathEffect(transform_.translate, baseColor_);
      if (camera_) {
        if (auto railCamera = dynamic_cast<const RailCamera*>(camera_)) {
            const_cast<RailCamera*>(railCamera)->Shake(1.0f, 0.3f);
        }
      }
    }

    if (onDestroyedCallback_) {
      onDestroyedCallback_(isSelfDestruct);
    }
    Destroy();
  }
}


