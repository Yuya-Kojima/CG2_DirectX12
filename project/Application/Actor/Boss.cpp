#include "Actor/Boss.h"
#include "Camera/ICamera.h"
#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Collision/CollisionConfig.h"
#include "Math/MathUtil.h"
#include "Render/Object3d/Object3d.h"
#include "Actor/Player.h" 
#include "Debug/Logger.h"
#include "Actor/EnemyBullet.h"
#include "Framework/ActorManager.h"
#include "Framework/PrefabManager.h"
#include "Camera/RailCamera.h"
#include <cmath>
#include <algorithm>
#include "Framework/GameManager.h"

Boss::Boss() {}

Boss::~Boss() {}

void Boss::Initialize() {
  Enemy::Initialize();
  if (collider_) {
    collider_->SetRadius(1.0f); // モデルのサイズに合わせる
  }
  SetHP(100);
  maxHp_ = 100;
  basePosition_ = transform_.translate;
  phase_ = BossPhase::Phase1;
  isDead_ = false;

  aliveTime_ = 0.0f;
  shotTimer_ = 0.0f;
  actionTimer_ = 0.0f;
  isCharging_ = false;
  dyingTimer_ = 0.0f;
  nextExplosionTime_ = 0.0f;

  if (model_) {
    model_->SetColor(baseColor_);
  }
}

void Boss::InitializeUI(SpriteRenderer* spriteRenderer) {
  if (!spriteRenderer) return;

  hpBarBg_ = std::make_unique<Sprite>();
  hpBarBg_->Initialize(spriteRenderer, "resources/white1x1.png");
  hpBarBg_->SetAnchorPoint({0.5f, 0.5f});
  hpBarBg_->SetPosition({1280.0f / 2.0f, 60.0f}); // 画面上部中央
  hpBarBg_->SetSize({800.0f, 20.0f});
  hpBarBg_->SetColor({0.2f, 0.2f, 0.2f, 0.8f});

  hpBarFg_ = std::make_unique<Sprite>();
  hpBarFg_->Initialize(spriteRenderer, "resources/white1x1.png");
  hpBarFg_->SetAnchorPoint({0.0f, 0.5f}); // スケール用に左端アンカー
  hpBarFg_->SetPosition({(1280.0f / 2.0f) - 400.0f, 60.0f});
  hpBarFg_->SetSize({800.0f, 20.0f});
  hpBarFg_->SetColor({1.0f, 0.2f, 0.2f, 1.0f});

  isUIInitialized_ = true;
}

void Boss::Update() {
  if (isDead_) return;

  // --- Dyingフェーズ処理 ---
  if (phase_ == BossPhase::Dying) {
    dyingTimer_ += 1.0f / 60.0f;

    // 一定間隔で爆発エフェクトのコールバックを発火
    if (dyingTimer_ >= nextExplosionTime_) {
      // 爆発位置をランダムにボス周辺に散らす
      float rx = (static_cast<float>(rand() % 200) - 100.0f) / 100.0f * 4.0f;
      float ry = (static_cast<float>(rand() % 200) - 100.0f) / 100.0f * 4.0f;
      float rz = (static_cast<float>(rand() % 200) - 100.0f) / 100.0f * 4.0f;
      Vector3 explosionPos = {transform_.translate.x + rx,
                              transform_.translate.y + ry,
                              transform_.translate.z + rz};
      if (onExplosionCallback_) {
        onExplosionCallback_(explosionPos);
      }
      // 爆発間隔を徐々に短くして激しくなる演出
      float progress = dyingTimer_ / dyingDuration_;
      float interval = 0.5f * (1.0f - progress * 0.8f); // 0.5秒→0.1秒に短縮
      nextExplosionTime_ = dyingTimer_ + interval;
    }

    // 演出終了 → 完全消滅
    if (dyingTimer_ >= dyingDuration_) {
      ChangePhase(BossPhase::Defeated);
    }
    return; // Dyingフェーズ中は攻撃・移動をしない
  }

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

  aliveTime_ += 1.0f / 60.0f;

  if (phase_ == BossPhase::Phase1) {
    // フェーズ1の動き: ゆっくりホバリング
    transform_.translate.x = basePosition_.x + std::sin(aliveTime_ * 0.5f) * 8.0f;
    transform_.translate.y = basePosition_.y + std::cos(aliveTime_ * 0.8f) * 1.5f;
    transform_.translate.z = basePosition_.z;

    // プレイヤーの方向を向く
    if (player_) {
      Vector3 playerPos = player_->GetTransform().translate;
      Vector3 dirToPlayer = {playerPos.x - transform_.translate.x,
                             playerPos.y - transform_.translate.y,
                             playerPos.z - transform_.translate.z};
      transform_.rotate.y = std::atan2(dirToPlayer.x, dirToPlayer.z) + 3.14159265f;
    }

    // 4秒ごとに3点バースト弾
    shotTimer_ += 1.0f / 60.0f;
    if (shotTimer_ >= 4.0f) {
      shotTimer_ = 0.0f;
      if (player_) {
        Vector3 playerPos = player_->GetTransform().translate;
        Vector3 myPos = transform_.translate;
        Vector3 dir = {playerPos.x - myPos.x, playerPos.y - myPos.y, playerPos.z - myPos.z};
        float dist = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        if (dist > 0.001f) {
          dir.x /= dist; dir.y /= dist; dir.z /= dist;
          float speed = 8.0f;
          
          Vector3 velCenter = {dir.x * speed, dir.y * speed, dir.z * speed};
          float angle = 15.0f * (3.14159265f / 180.0f);
          Vector3 velLeft = {
            (dir.x * std::cos(angle) + dir.z * std::sin(angle)) * speed,
            dir.y * speed,
            (-dir.x * std::sin(angle) + dir.z * std::cos(angle)) * speed
          };
          Vector3 velRight = {
            (dir.x * std::cos(-angle) + dir.z * std::sin(-angle)) * speed,
            dir.y * speed,
            (-dir.x * std::sin(-angle) + dir.z * std::cos(-angle)) * speed
          };

          auto spawnBullet = [&](const Vector3& vel) {
            auto bullet = std::make_unique<EnemyBullet>();
            bullet->Initialize(PrefabManager::GetInstance()->GetObject3dRenderer(), myPos, vel, const_cast<Player*>(player_));
            ActorManager::GetInstance()->AddActor(std::move(bullet));
          };
          spawnBullet(velCenter);
          spawnBullet(velLeft);
          spawnBullet(velRight);

          if (camera_) {
             auto railCam = static_cast<RailCamera*>(const_cast<ICamera*>(camera_));
             railCam->Shake(0.3f, 0.1f);
          }
        }
      }
    }
  } else if (phase_ == BossPhase::Phase2) {
    // フェーズ2の動き: 激しいホバリング
    transform_.translate.x = basePosition_.x + std::sin(aliveTime_ * 1.5f) * 12.0f;
    transform_.translate.y = basePosition_.y + std::cos(aliveTime_ * 1.0f) * 2.0f;
    transform_.translate.z = basePosition_.z;

    // プレイヤーの方向を向く
    if (player_) {
      Vector3 playerPos = player_->GetTransform().translate;
      Vector3 dirToPlayer = {playerPos.x - transform_.translate.x,
                             playerPos.y - transform_.translate.y,
                             playerPos.z - transform_.translate.z};
      transform_.rotate.y = std::atan2(dirToPlayer.x, dirToPlayer.z) + 3.14159265f;
    }

    // 5秒ごとに全方位8方向弾
    shotTimer_ += 1.0f / 60.0f;
    if (shotTimer_ >= 5.0f) {
      shotTimer_ = 0.0f;
      Vector3 myPos = transform_.translate;
      float speed = 10.0f;
      for (int i = 0; i < 8; ++i) {
        float rad = (i * 45.0f) * (3.14159265f / 180.0f);
        Vector3 vel = {std::sin(rad) * speed, 0.0f, std::cos(rad) * speed};
        auto bullet = std::make_unique<EnemyBullet>();
        bullet->Initialize(PrefabManager::GetInstance()->GetObject3dRenderer(), myPos, vel, const_cast<Player*>(player_));
        ActorManager::GetInstance()->AddActor(std::move(bullet));
      }
    }

    // 定期的な突進(Z軸接近)
    if (!isCharging_) {
      actionTimer_ += 1.0f / 60.0f;
      if (actionTimer_ >= 4.0f) {
        isCharging_ = true;
        actionTimer_ = 0.0f;
        chargeTime_ = 0.0f;
        chargeDuration_ = 1.0f;
        chargeStartPos_ = basePosition_;
        chargeTargetPos_ = basePosition_;
        chargeTargetPos_.z -= 40.0f; // 前に40ユニット迫る

        if (camera_) {
           auto railCam = static_cast<RailCamera*>(const_cast<ICamera*>(camera_));
           railCam->Shake(0.8f, 0.3f);
        }
      }
    } else {
      chargeTime_ += 1.0f / 60.0f;
      float t = std::min(chargeTime_ / chargeDuration_, 1.0f);
      float easeT = 1.0f - std::pow(1.0f - t, 3.0f);
      basePosition_.z = chargeStartPos_.z + (chargeTargetPos_.z - chargeStartPos_.z) * easeT;

      if (t >= 1.0f) {
        if (actionTimer_ == 0.0f) {
          actionTimer_ = 1.0f;
          chargeTime_ = 0.0f;
          chargeStartPos_ = basePosition_;
          chargeTargetPos_ = basePosition_;
          chargeTargetPos_.z += 40.0f; // 元に戻る
        } else {
          isCharging_ = false;
          actionTimer_ = 0.0f;
        }
      }
    }
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
  if (model_) {
    model_->Draw();
  }
}

void Boss::DrawUI() {
  if (!isUIInitialized_ || isDead_) return;

  // HPの割合でスケール(Size)を更新
  float hpRatio = static_cast<float>(hp_) / maxHp_;
  if (hpRatio < 0.0f) hpRatio = 0.0f;
  
  hpBarFg_->SetSize({800.0f * hpRatio, 20.0f});

  // 色もフェーズによって変える（例: Phase2でオレンジ色に）
  if (phase_ == BossPhase::Phase2) {
    hpBarFg_->SetColor({1.0f, 0.6f, 0.0f, 1.0f});
  } else {
    hpBarFg_->SetColor({1.0f, 0.2f, 0.2f, 1.0f});
  }

  Transform defaultUv;
  defaultUv.scale = {1.0f, 1.0f, 1.0f};
  defaultUv.rotate = {0.0f, 0.0f, 0.0f};
  defaultUv.translate = {0.0f, 0.0f, 0.0f};
  
  hpBarBg_->Update(defaultUv);
  hpBarFg_->Update(defaultUv);

  hpBarBg_->Draw();
  hpBarFg_->Draw();
}

void Boss::OnCollision(Collider *other) {
  // Bossの場合は自爆しないので呼ばない
}

void Boss::TakeDamage(int damage, bool isSelfDestruct) {
  if (isDead_) return;

  hp_ -= damage;
  hitFlashTimer_ = 5;

  // フェーズ移行判定 (T-2)
  if (phase_ == BossPhase::Phase1 && hp_ <= maxHp_ / 2) {
    ChangePhase(BossPhase::Phase2);
  }

  if (hp_ <= 0) {
    hp_ = 0;
    ChangePhase(BossPhase::Dying); // 即消滅ではなくDyingフェーズへ
  }
}

void Boss::ChangePhase(BossPhase nextPhase) {
  phase_ = nextPhase;
  if (phase_ == BossPhase::Phase2) {
    Logger::Log("Boss entering Phase 2!\n");
    // パターン変化の初期化など
  } else if (phase_ == BossPhase::Dying) {
    Logger::Log("Boss entering Dying phase!\n");
    // コライダーを無効化（当たり判定を消す）
    // CollisionManagerが遅延削除に対応したため、ここでRemoveを呼んでも安全です
    if (collider_) {
      CollisionManager::GetInstance()->Remove(collider_.get());
      collider_.reset();
    }
    dyingTimer_ = 0.0f;
    nextExplosionTime_ = 0.0f;
    // HPバーを非表示にする
    isUIInitialized_ = false;
  } else if (phase_ == BossPhase::Defeated) {
    Logger::Log("Boss Defeated!\n");
    // スコア加算
    GameManager::GetInstance()->AddScore(10000);
    
    if (onDestroyedCallback_) {
      onDestroyedCallback_(false);
    }
    isDead_ = true;
  }
}
