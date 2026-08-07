#include "Actor/Boss.h"
#include "Actor/EnemyBullet.h"
#include "Actor/Player.h"
#include "Camera/ICamera.h"
#include "Camera/RailCamera.h"
#include "Collision/CollisionConfig.h"
#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Debug/Logger.h"
#include "Framework/ActorManager.h"
#include "Framework/GameManager.h"
#include "Framework/PrefabManager.h"
#include "Math/MathUtil.h"
#include "Render/Object3d/Object3d.h"
#include "Render/Texture/TextureManager.h"
#include <algorithm>
#include <cmath>

Boss::Boss() {}

Boss::~Boss() {}

void Boss::Initialize() {
  Enemy::Initialize();
  if (collider_) {
    collider_->SetRadius(1.5f); // 巨大化に合わせて大きめの当たり判定に変更
  }
  SetHP(100);
  maxHp_ = 100;
  phase_ = BossPhase::Phase1;
  isDead_ = false;

  currentState_ = BossState::Enter;
  stateTimer_ = 0.0f;
  startPos_ = {0.0f, 0.0f, 0.0f};
  targetPos_ = {0.0f, 0.0f, 0.0f};

  aliveTime_ = 0.0f;
  dyingTimer_ = 0.0f;

  dissolveEnabled_ = false;
}

void Boss::InitializeUI(SpriteRenderer *spriteRenderer) {
  if (!spriteRenderer)
    return;

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
  if (isDead_) {
    return;
  }

  // 被弾時のフラッシュ処理
  if (hitFlashTimer_ > 0) {
    hitFlashTimer_--;
    if (model_) {
      model_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤色に点滅
    }
  } else {
    // 状態に応じたカラーの適用
    if (model_) {
      if (currentState_ == BossState::Telegraph) {
        model_->SetColor({1.0f, 0.5f, 0.0f, 1.0f}); // 予兆時はオレンジ
      } else {
        model_->SetColor(baseColor_);
      }
    }
  }

  aliveTime_ += 1.0f / 60.0f;

  if (phase_ == BossPhase::Phase1) {
    UpdatePhase1();
  } else if (phase_ == BossPhase::Phase2) {
    UpdatePhase2();
  } else if (phase_ == BossPhase::Dying) {
    UpdateDying();
  }

  // 最後にトランスフォーム（モデル・コライダー位置）を更新する
  UpdateTransform();
}

void Boss::UpdatePhase1() {
  stateTimer_ += 1.0f / 60.0f;

  switch (currentState_) {
  case BossState::Enter:
    // カメラの前方（基準位置）に配置
    if (camera_) {
        Vector3 cPos = camera_->GetTranslate();
        Vector3 cRight = camera_->GetRight();
        Vector3 cUp = camera_->GetUp();
        Vector3 cForward = camera_->GetForward();
        if (auto railCam = dynamic_cast<const RailCamera*>(camera_)) {
            cPos = railCam->GetRailPosition();
            cRight = railCam->GetRailRight();
            cUp = railCam->GetRailUp();
            cForward = railCam->GetRailForward();
        }
        Vector3 center = cPos + cRight * spawnOffset_.x + cUp * spawnOffset_.y + cForward * spawnOffset_.z;
        transform_.translate = center;
        startPos_ = center;
        targetPos_ = center;
    }
    currentState_ = BossState::Hover;
    stateTimer_ = 0.0f;
    break;

  case BossState::Hover: {
    // イージング（EaseOutCubic）を用いて targetPos_ へ滑らかに移動
    float t = std::min(stateTimer_ / 3.0f, 1.0f);
    float easeT = 1.0f - std::pow(1.0f - t, 3.0f); 
    
    transform_.translate.x = startPos_.x + (targetPos_.x - startPos_.x) * easeT;
    transform_.translate.y = startPos_.y + (targetPos_.y - startPos_.y) * easeT;
    transform_.translate.z = startPos_.z + (targetPos_.z - startPos_.z) * easeT;

    // 常にプレイヤーの方向を向く
    if (player_) {
      Vector3 pPos = player_->GetTransform().translate;
      Vector3 dir = {pPos.x - transform_.translate.x, pPos.y - transform_.translate.y, pPos.z - transform_.translate.z};
      transform_.rotate.y = std::atan2(dir.x, dir.z) + 3.14159265f;
    }

    // 3秒経過したら予兆ステートへ
    if (stateTimer_ >= 3.0f) {
      currentState_ = BossState::Telegraph;
      stateTimer_ = 0.0f;
    }
    break;
  }

  case BossState::Telegraph: {
    // 予兆：小刻みに震える
    // 基準位置（targetPos_）にランダムな微小オフセットを加算
    float shakeAmount = 0.2f;
    float rx = ((rand() % 100) / 100.0f - 0.5f) * shakeAmount;
    float ry = ((rand() % 100) / 100.0f - 0.5f) * shakeAmount;
    float rz = ((rand() % 100) / 100.0f - 0.5f) * shakeAmount;
    transform_.translate.x = targetPos_.x + rx;
    transform_.translate.y = targetPos_.y + ry;
    transform_.translate.z = targetPos_.z + rz;

    // 1秒経過したら攻撃ステートへ
    if (stateTimer_ >= 1.0f) {
      // 震えを元に戻す
      transform_.translate = targetPos_;
      currentState_ = BossState::Attack;
      stateTimer_ = 0.0f;
    }
    break;
  }

  case BossState::Attack: {
    // プレイヤーの方向を計算して扇状に弾を発射
    if (player_) {
      Vector3 playerPos = player_->GetTransform().translate;
      Vector3 myPos = transform_.translate;
      Vector3 dir = {playerPos.x - myPos.x, playerPos.y - myPos.y, playerPos.z - myPos.z};
      float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
      if (dist > 0.001f) {
        dir.x /= dist;
        dir.y /= dist;
        dir.z /= dist;
        
        auto spawnBullet = [&](const Vector3 &vel, EnemyBulletType type) {
          auto bullet = std::make_unique<EnemyBullet>();
          bullet->Initialize(
              PrefabManager::GetInstance()->GetObject3dRenderer(), myPos, vel,
              const_cast<Player *>(player_), type);
          ActorManager::GetInstance()->AddActor(std::move(bullet));
        };

        // 扇状に5発の通常弾を発射
        float normalSpeed = 0.5f;
        for (int i = 0; i < 5; ++i) {
          float angle = (-20.0f + i * 10.0f) * (3.14159265f / 180.0f);
          Vector3 vel = {
              (dir.x * std::cos(angle) + dir.z * std::sin(angle)) * normalSpeed,
              dir.y * normalSpeed,
              (-dir.x * std::sin(angle) + dir.z * std::cos(angle)) * normalSpeed};
          spawnBullet(vel, EnemyBulletType::NormalDestructible);
        }
        
        // 発射時のカメラ揺れ
        if (camera_) {
          if (auto railCam = dynamic_cast<const RailCamera*>(camera_)) {
            // Const外しを避けるためにconst_cast（元のコード踏襲）
            auto mutableRailCam = static_cast<RailCamera *>(const_cast<ICamera *>(camera_));
            mutableRailCam->Shake(0.3f, 0.1f);
          }
        }
      }
    }
    
    // 発射後、即座に硬直ステートへ
    currentState_ = BossState::Cooldown;
    stateTimer_ = 0.0f;
    break;
  }

  case BossState::Cooldown:
    // 2秒経過したら再びHoverに戻ってループ
    // 新しい目標座標（targetPos_）を計算してセット
    if (stateTimer_ >= 2.0f) {
      startPos_ = targetPos_;
      
      // カメラを基準にした新しい移動先をランダムに決定
      if (camera_) {
          Vector3 cPos = camera_->GetTranslate();
          Vector3 cRight = camera_->GetRight();
          Vector3 cUp = camera_->GetUp();
          Vector3 cForward = camera_->GetForward();
          if (auto railCam = dynamic_cast<const RailCamera*>(camera_)) {
              cPos = railCam->GetRailPosition();
              cRight = railCam->GetRailRight();
              cUp = railCam->GetRailUp();
              cForward = railCam->GetRailForward();
          }
          Vector3 center = cPos + cRight * spawnOffset_.x + cUp * spawnOffset_.y + cForward * spawnOffset_.z;
          
          // ランダムに左右(X)・上下(Y)に少しずらした位置を次の目標にする
          float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 20.0f; // -10 から +10
          float offsetY = ((rand() % 100) / 100.0f - 0.5f) * 10.0f; // -5 から +5
          
          targetPos_.x = center.x + offsetX;
          targetPos_.y = center.y + offsetY;
          targetPos_.z = center.z; // Z軸（奥への距離）は基準位置をキープ
      }

      currentState_ = BossState::Hover;
      stateTimer_ = 0.0f;
    }
    break;
  }
}

void Boss::UpdatePhase2() {
  stateTimer_ += 1.0f / 60.0f;

  switch (currentState_) {
  case BossState::Enter:
  case BossState::Hover: {
    // 高速・広範囲なイージング移動
    float t = std::min(stateTimer_ / 1.5f, 1.0f);
    float easeT = 1.0f - std::pow(1.0f - t, 3.0f); 
    
    transform_.translate.x = startPos_.x + (targetPos_.x - startPos_.x) * easeT;
    transform_.translate.y = startPos_.y + (targetPos_.y - startPos_.y) * easeT;
    transform_.translate.z = startPos_.z + (targetPos_.z - startPos_.z) * easeT;

    if (player_) {
      Vector3 pPos = player_->GetTransform().translate;
      Vector3 dir = {pPos.x - transform_.translate.x, pPos.y - transform_.translate.y, pPos.z - transform_.translate.z};
      transform_.rotate.y = std::atan2(dir.x, dir.z) + 3.14159265f;
    }

    if (stateTimer_ >= 1.5f) {
      if (attackStep_ % 2 == 0) {
        currentState_ = BossState::Telegraph; // ミサイル攻撃へ
      } else {
        currentState_ = BossState::DashTelegraph; // 突進攻撃へ
      }
      stateTimer_ = 0.0f;
    }
    break;
  }

  case BossState::Telegraph: {
    // 激しい予兆（ミサイル用）
    float shakeAmount = 0.4f;
    float rx = ((rand() % 100) / 100.0f - 0.5f) * shakeAmount;
    float ry = ((rand() % 100) / 100.0f - 0.5f) * shakeAmount;
    float rz = ((rand() % 100) / 100.0f - 0.5f) * shakeAmount;
    transform_.translate.x = targetPos_.x + rx;
    transform_.translate.y = targetPos_.y + ry;
    transform_.translate.z = targetPos_.z + rz;

    if (stateTimer_ >= 0.5f) {
      transform_.translate = targetPos_;
      currentState_ = BossState::Attack;
      stateTimer_ = 0.0f;
    }
    break;
  }

  case BossState::Attack: {
    if (player_) {
      Vector3 playerPos = player_->GetTransform().translate;
      Vector3 myPos = transform_.translate;
      Vector3 dir = {playerPos.x - myPos.x, playerPos.y - myPos.y, playerPos.z - myPos.z};
      float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
      if (dist > 0.001f) {
        dir.x /= dist; dir.y /= dist; dir.z /= dist;
        auto spawnBullet = [&](const Vector3 &vel, EnemyBulletType type) {
          auto bullet = std::make_unique<EnemyBullet>();
          bullet->Initialize(PrefabManager::GetInstance()->GetObject3dRenderer(), myPos, vel, const_cast<Player *>(player_), type);
          ActorManager::GetInstance()->AddActor(std::move(bullet));
        };

        // ロックオン用ミサイルを5発ばら撒く
        float missileSpeed = 4.0f;
        for (int i = 0; i < 5; ++i) {
          float angle = (-80.0f + i * 40.0f) * (3.14159265f / 180.0f);
          Vector3 vel = {
              (dir.x * std::cos(angle) + dir.z * std::sin(angle)) * missileSpeed,
              dir.y * missileSpeed - 2.0f,
              (-dir.x * std::sin(angle) + dir.z * std::cos(angle)) * missileSpeed};
          spawnBullet(vel, EnemyBulletType::LockOnDestructible);
        }
        
        if (camera_) {
          if (auto railCam = dynamic_cast<const RailCamera*>(camera_)) {
            auto mutableRailCam = static_cast<RailCamera *>(const_cast<ICamera *>(camera_));
            mutableRailCam->Shake(0.5f, 0.15f);
          }
        }
      }
    }
    currentState_ = BossState::Cooldown;
    stateTimer_ = 0.0f;
    break;
  }

  case BossState::Cooldown: {
    if (stateTimer_ >= 1.0f) {
      attackStep_++;
      startPos_ = targetPos_;
      if (camera_) {
          Vector3 cPos = camera_->GetTranslate();
          Vector3 cRight = camera_->GetRight();
          Vector3 cUp = camera_->GetUp();
          Vector3 cForward = camera_->GetForward();
          if (auto railCam = dynamic_cast<const RailCamera*>(camera_)) {
              cPos = railCam->GetRailPosition();
              cRight = railCam->GetRailRight();
              cUp = railCam->GetRailUp();
              cForward = railCam->GetRailForward();
          }
          Vector3 center = cPos + cRight * spawnOffset_.x + cUp * spawnOffset_.y + cForward * spawnOffset_.z;
          float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 24.0f;
          float offsetY = ((rand() % 100) / 100.0f - 0.5f) * 12.0f;
          targetPos_.x = center.x + offsetX;
          targetPos_.y = center.y + offsetY;
          targetPos_.z = center.z; 
      }
      currentState_ = BossState::Hover;
      stateTimer_ = 0.0f;
    }
    break;
  }

  case BossState::DashTelegraph: {
    // 突進前のタメ動作（大きく震える）
    float shakeAmount = 0.6f;
    float rx = ((rand() % 100) / 100.0f - 0.5f) * shakeAmount;
    float ry = ((rand() % 100) / 100.0f - 0.5f) * shakeAmount;
    float rz = ((rand() % 100) / 100.0f - 0.5f) * shakeAmount;
    transform_.translate.x = targetPos_.x + rx;
    transform_.translate.y = targetPos_.y + ry;
    transform_.translate.z = targetPos_.z + rz;

    if (stateTimer_ >= 0.8f) {
      transform_.translate = targetPos_;
      startPos_ = targetPos_;
      // 突進の目標座標を現在位置からZ方向手前に40ユニット迫った場所に設定
      targetPos_.z -= 40.0f; 

      if (camera_) {
        if (auto railCam = dynamic_cast<const RailCamera*>(camera_)) {
          auto mutableRailCam = static_cast<RailCamera *>(const_cast<ICamera *>(camera_));
          mutableRailCam->Shake(0.8f, 0.3f); // 突進開始時の大きな揺れ
        }
      }

      currentState_ = BossState::Dash;
      stateTimer_ = 0.0f;
    }
    break;
  }

  case BossState::Dash: {
    // 0.5秒で目標(Z-40)へ急接近
    float t = std::min(stateTimer_ / 0.5f, 1.0f);
    float easeT = t * t; // EaseIn
    
    transform_.translate.x = startPos_.x + (targetPos_.x - startPos_.x) * easeT;
    transform_.translate.y = startPos_.y + (targetPos_.y - startPos_.y) * easeT;
    transform_.translate.z = startPos_.z + (targetPos_.z - startPos_.z) * easeT;

    if (stateTimer_ >= 0.5f) {
      currentState_ = BossState::DashCooldown;
      stateTimer_ = 0.0f;
    }
    break;
  }

  case BossState::DashCooldown: {
    // 突進後、1.5秒間隙を晒す
    if (stateTimer_ >= 1.5f) {
      attackStep_++;
      startPos_ = targetPos_;
      
      // 元のZ位置（奥）に戻るためのHover目標を設定
      if (camera_) {
          Vector3 cPos = camera_->GetTranslate();
          Vector3 cRight = camera_->GetRight();
          Vector3 cUp = camera_->GetUp();
          Vector3 cForward = camera_->GetForward();
          if (auto railCam = dynamic_cast<const RailCamera*>(camera_)) {
              cPos = railCam->GetRailPosition();
              cRight = railCam->GetRailRight();
              cUp = railCam->GetRailUp();
              cForward = railCam->GetRailForward();
          }
          Vector3 center = cPos + cRight * spawnOffset_.x + cUp * spawnOffset_.y + cForward * spawnOffset_.z;
          float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 24.0f;
          float offsetY = ((rand() % 100) / 100.0f - 0.5f) * 12.0f;
          targetPos_.x = center.x + offsetX;
          targetPos_.y = center.y + offsetY;
          targetPos_.z = center.z; // Z軸を元の距離に戻す
      }
      currentState_ = BossState::Hover;
      stateTimer_ = 0.0f;
    }
    break;
  }
  }
}

void Boss::UpdateDying() {
  dyingTimer_ += 1.0f / 60.0f;

  // ディゾルブ進行
  if (model_) {
    if (!dissolveEnabled_ && dyingTimer_ > 0.0f) {
      model_->SetEnableDissolve(true);
      dissolveEnabled_ = true;
    }
    float progress = dyingTimer_ / dyingDuration_;
    progress = std::min(progress, 1.0f);
    model_->SetDissolveThreshold(progress);
  }

  // 塵パーティクルの位置更新
  float progressForDust = dyingTimer_ / dyingDuration_;
  if (onDyingUpdateCallback_ && progressForDust < 0.6f) {
    onDyingUpdateCallback_(transform_.translate);
  }

  // 演出終了 → 完全消滅
  if (dyingTimer_ >= dyingDuration_) {
    ChangePhase(BossPhase::Defeated);
  }
}

void Boss::Draw3D() {
  if (model_) {
    model_->Draw();
  }
}

void Boss::Draw2D() {
  if (!isUIInitialized_ || isDead_)
    return;

  // HPの割合でスケール(Size)を更新
  float hpRatio = static_cast<float>(hp_) / maxHp_;
  if (hpRatio < 0.0f)
    hpRatio = 0.0f;

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
  if (other->GetOwner() && dynamic_cast<Player*>(other->GetOwner())) {
    Player* p = dynamic_cast<Player*>(other->GetOwner());
    if (p->GetInvincibleTimer() > 0) {
      return; // 無敵中は食らわない
    }
    p->TakeDamage(1); // プレイヤーにダメージを与える
  }
}

void Boss::TakeDamage(int damage, bool isSelfDestruct) {
  if (isDead_)
    return;

  hp_ -= damage;
  hitFlashTimer_ = 5;

  // フェーズ移行判定 (T-2)
  if (hp_ <= 0) {
    hp_ = 0;
    ChangePhase(BossPhase::Dying); // 即消滅ではなくDyingフェーズへ
  } else if (phase_ == BossPhase::Phase1 && hp_ <= maxHp_ / 2) {
    ChangePhase(BossPhase::Phase2);
  }
}

void Boss::ChangePhase(BossPhase nextPhase) {
  phase_ = nextPhase;
  if (phase_ == BossPhase::Phase2) {
    Logger::Log("Boss entering Phase 2!\n");
    // パターン変化の初期化など
    currentState_ = BossState::Hover;
    stateTimer_ = 0.0f;
    attackStep_ = 0;
    startPos_ = transform_.translate;
    targetPos_ = transform_.translate;
  } else if (phase_ == BossPhase::Dying) {
    Logger::Log("Boss entering Dying phase!\n");
    // コライダーを無効化（メモリは破棄しない）
    if (collider_) {
      collider_->SetEnable(false);
    }
    dyingTimer_ = 0.0f;
    // HPバーを非表示にする
    isUIInitialized_ = false;
  } else if (phase_ == BossPhase::Defeated) {
    Logger::Log("Boss Defeated!\n");
    onDyingUpdateCallback_ = nullptr; // 塵の放出を止める

    if (onDestroyedCallback_) {
      onDestroyedCallback_(false);
    }
    isDead_ = true;
  }
}
