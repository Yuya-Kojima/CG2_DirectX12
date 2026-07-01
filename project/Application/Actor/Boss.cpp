#include "Actor/Boss.h"
#include "Camera/ICamera.h"
#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Collision/CollisionConfig.h"
#include "Math/MathUtil.h"
#include "Render/Object3d/Object3d.h"
#include "Actor/Player.h" 
#include "Debug/Logger.h"

Boss::Boss() {
  // ボスのコライダーの初期化
  collider_ = std::make_unique<SphereCollider>(this);
  collider_->SetRadius(3.0f); // ボスは大きめ
  collider_->SetAttribute(kCollisionAttributeEnemy); // 自分は「敵」扱い
  collider_->SetMask(kCollisionAttributePlayer |
                     kCollisionAttributePlayerBullet); // 自機や自機の弾と当たる
  CollisionManager::GetInstance()->Register(collider_.get());
}

Boss::~Boss() {
  if (collider_) {
    CollisionManager::GetInstance()->Remove(collider_.get());
  }
}

void Boss::Initialize() {
  hp_ = maxHp_;
  phase_ = BossPhase::Phase1;
  isDead_ = false;

  if (model_) {
    model_->SetColor(baseColor_);
  }
}

void Boss::Update() {
  if (isDead_) return;

  // 被弾時のフラッシュ処理
  if (hitFlashTimer_ > 0) {
    hitFlashTimer_--;
    if (model_) {
      model_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤色に点滅
    }
  } else {
    if (model_) {
      model_->SetColor(baseColor_);
    }
  }

  // TODO: T-3, T-6 ここにフェーズごとの行動パターンを記述
  if (phase_ == BossPhase::Phase1) {
    // フェーズ1の動き
  } else if (phase_ == BossPhase::Phase2) {
    // フェーズ2の動き
  }
}

void Boss::UpdateTransform() {
  BaseActor::UpdateTransform();
  
  if (model_) {
    model_->SetTranslation(transform_.translate);
    model_->SetRotation(transform_.rotate);
    model_->SetScale(transform_.scale);
    model_->Update();
  }
}

void Boss::Draw3D() {
  if (model_ && !isDead_) {
    model_->Draw();
  }
}

void Boss::OnCollision(Collider *other) {
  // プレイヤーの弾や自機との衝突処理
  // プレイヤー側の処理で TakeDamage が呼ばれる想定なのでここは基本空でもOK
}

void Boss::TakeDamage(int damage) {
  if (isDead_) return;

  hp_ -= damage;
  hitFlashTimer_ = 5;

  // フェーズ移行判定 (T-2)
  if (phase_ == BossPhase::Phase1 && hp_ <= maxHp_ / 2) {
    ChangePhase(BossPhase::Phase2);
  }

  if (hp_ <= 0) {
    hp_ = 0;
    ChangePhase(BossPhase::Defeated);
  }
}

void Boss::ChangePhase(BossPhase nextPhase) {
  phase_ = nextPhase;
  if (phase_ == BossPhase::Phase2) {
    Logger::Log("Boss entering Phase 2!\n");
    // パターン変化の初期化など
  } else if (phase_ == BossPhase::Defeated) {
    Logger::Log("Boss Defeated!\n");
    if (onDestroyedCallback_) {
      onDestroyedCallback_();
    }
    isDead_ = true;
  }
}
