#include "EnemyBullet.h"
#include "Actor/HomingBullet.h"
#include "Actor/NormalBullet.h"
#include "Actor/Player.h"
#include "Collision/CollisionConfig.h"
#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Debug/Logger.h"
#include "Effect/EffectManager.h"
#include "Math/MathUtil.h"
#include "Render/Object3d/Object3d.h"
#include <cmath>

EnemyBullet::EnemyBullet() {}
EnemyBullet::~EnemyBullet() {
  if (collider_) {
    CollisionManager::GetInstance()->Remove(collider_.get());
  }
}

void EnemyBullet::Initialize(Object3dRenderer *renderer,
                             const Vector3 &startPos, const Vector3 &velocity,
                             Player *player, EnemyBulletType type) {
  object3d_ = std::make_unique<Object3d>();
  object3d_->Initialize(renderer);

  type_ = type;
  velocity_ = velocity;
  player_ = player;
  lifeTimer_ = 1800; // 約30秒で消滅（安全装置としての寿命）

  // タイプごとの見た目とHPの設定
  object3d_->SetModel("suzanne.obj");
  if (type_ == EnemyBulletType::NormalDestructible) {
    // 通常ショットで壊す細かい弾
    object3d_->SetScale({1.0f, 1.0f, 1.0f});
    object3d_->SetColor({1.0f, 0.5f, 0.0f, 1.0f}); // オレンジ
    hp_ = 1;
  } else if (type_ == EnemyBulletType::LockOnDestructible) {
    // スウォームミサイル
    object3d_->SetScale({2.0f, 2.0f, 2.0f});       // 少し小さく
    object3d_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤
    hp_ = 1;
  } else {
    // 破壊不可エネルギー弾
    object3d_->SetScale({1.5f, 1.5f, 3.0f});
    object3d_->SetColor({0.0f, 0.0f, 1.0f, 1.0f}); // 青
    hp_ = 9999;
  }

  object3d_->SetTranslation(startPos);

  // コライダーの設定
  collider_ = std::make_unique<SphereCollider>(this);
  collider_->SetRadius(1.5f * object3d_->GetScale().x);
  collider_->SetAttribute(kCollisionAttributeEnemyBullet);
  // プレイヤー自身と、プレイヤーの弾の両方と衝突判定を行う
  collider_->SetMask(kCollisionAttributePlayer |
                     kCollisionAttributePlayerBullet);
  collider_->SetVelocity(velocity_);
  CollisionManager::GetInstance()->Register(collider_.get());
}

void EnemyBullet::Update() {
  if (isDead_)
    return;

  lifeTimer_--;
  if (lifeTimer_ <= 0) {
    isDead_ = true;
  }

  // 描画限界距離（カリング距離）による消滅判定
  if (player_ && !player_->IsDead()) {
    Vector3 currentPos = object3d_->GetTranslation();
    Vector3 playerPos = player_->GetTransform().translate;
    Vector3 toPlayer = {playerPos.x - currentPos.x, playerPos.y - currentPos.y,
                        playerPos.z - currentPos.z};
    float distSq = toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y +
                   toPlayer.z * toPlayer.z;
    if (distSq > 300.0f * 300.0f) { // プレイヤーから距離300以上離れたら消滅
      isDead_ = true;
      return;
    }
  }

  // 位置の更新の前に、ミサイル特有の誘導処理を入れる
  aliveFrames_++;
  if (type_ == EnemyBulletType::LockOnDestructible) {
    if (player_ && !player_->IsDead()) {
      Vector3 currentPos = object3d_->GetTranslation();
      Vector3 playerPos = player_->GetTransform().translate;

      Vector3 toPlayer = {playerPos.x - currentPos.x,
                          playerPos.y - currentPos.y,
                          playerPos.z - currentPos.z};
      float dist = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y +
                             toPlayer.z * toPlayer.z);

      if (dist > 0.001f) {
        toPlayer.x /= dist;
        toPlayer.y /= dist;
        toPlayer.z /= dist;
      }

      if (aliveFrames_ < swarmWaitFrames_) {
        int framesLeft = swarmWaitFrames_ - aliveFrames_;
        if (framesLeft == 1) {
          // タメから解放され、発射されるまさにその瞬間
          float burstSpeed = 2.5f;
          velocity_ = {toPlayer.x * burstSpeed, toPlayer.y * burstSpeed,
                       toPlayer.z * burstSpeed};

          // 発射時の衝撃波リングエフェクト（真っ白）を発生
          EffectManager::GetInstance()->PlayFunnelMuzzleRing(
              object3d_->GetTranslation(), {1.0f, 1.0f, 1.0f, 1.0f});
        } else if (framesLeft < 15) {
          // 発射直前の約0.25秒は完全に静止してタメを作る
          velocity_ = {0.0f, 0.0f, 0.0f};
        } else {
          // 展開中は急ブレーキをかける
          velocity_.x *= 0.85f;
          velocity_.y *= 0.85f;
          velocity_.z *= 0.85f;
          // ほんの少しだけ重力で落としてホバリング感を出す
          velocity_.y -= 0.01f;
        }
      } else {
        // 現在の進行方向と、プレイヤーへの方向の内積を計算
        Vector3 vNorm = velocity_;
        float vLen = std::sqrt(vNorm.x * vNorm.x + vNorm.y * vNorm.y +
                               vNorm.z * vNorm.z);
        if (vLen > 0.001f) {
          vNorm.x /= vLen;
          vNorm.y /= vLen;
          vNorm.z /= vLen;
        }
        float dot =
            vNorm.x * toPlayer.x + vNorm.y * toPlayer.y + vNorm.z * toPlayer.z;

        // プレイヤーに十分近づいた（距離40未満）かつすれ違ったなら誘導終了
        if (dist < 40.0f && dot < 0.0f) {
          homingStrength_ = -1.0f; // 負の数を入れて誘導終了フラグとする
        }

        // homingStrength_ が 0 以上なら誘導を続ける（-1ならそのまま直進）
        if (homingStrength_ >= 0.0f) {
          homingStrength_ += 0.010f;
          if (homingStrength_ > 0.10f)
            homingStrength_ = 0.10f;

          float missileSpeed = 1.1f; // ミサイルの最高速度を調整
          Vector3 desiredVelocity = {toPlayer.x * missileSpeed,
                                     toPlayer.y * missileSpeed,
                                     toPlayer.z * missileSpeed};

          velocity_.x = Lerp(velocity_.x, desiredVelocity.x, homingStrength_);
          velocity_.y = Lerp(velocity_.y, desiredVelocity.y, homingStrength_);
          velocity_.z = Lerp(velocity_.z, desiredVelocity.z, homingStrength_);
        }
      }
    }
  }

  // 位置の更新
  Vector3 pos = object3d_->GetTranslation();
  pos.x += velocity_.x;
  pos.y += velocity_.y;
  pos.z += velocity_.z;
  object3d_->SetTranslation(pos);
  transform_.translate = pos;

  // コライダーの更新
  if (collider_) {
    collider_->SetVelocity(velocity_);
  }

  object3d_->Update();
}

void EnemyBullet::OnCollision(Collider *other) {
  if (isDead_)
    return;

  // 相手がプレイヤーの場合
  if (other->GetOwner() && dynamic_cast<Player *>(other->GetOwner())) {
    Player *p = dynamic_cast<Player *>(other->GetOwner());
    if (p->GetInvincibleTimer() > 0) {
      return; // 無敵中はすり抜ける（消えない）
    }
    isDead_ = true;
    p->TakeDamage(1);
    return;
  }

  // 破壊不可弾は何が当たっても壊れない
  if (type_ == EnemyBulletType::Indestructible)
    return;

  // 相手がプレイヤーの弾かチェック
  if (other->GetAttribute() & kCollisionAttributePlayerBullet) {
    BaseActor *bulletOwner = other->GetOwner();

    // 相手が通常ショット（NormalBullet）の場合
    if (dynamic_cast<NormalBullet *>(bulletOwner)) {
      if (type_ == EnemyBulletType::NormalDestructible ||
          type_ == EnemyBulletType::LockOnDestructible) {
        hp_ -= 1; // 通常ショットで1ダメージ
        Logger::Log("EnemyBullet: Hit by NormalBullet!\n");
      }
    }
    // 相手がロックオンレーザー（HomingBullet）の場合
    else if (dynamic_cast<HomingBullet *>(bulletOwner)) {
      if (type_ == EnemyBulletType::NormalDestructible ||
          type_ == EnemyBulletType::LockOnDestructible) {
        hp_ -= 3; // ロックオンレーザーで3ダメージ（ミサイルを一撃破壊）
        Logger::Log("EnemyBullet: Hit by HomingBullet!\n");
      }
    }

    if (hp_ <= 0) {
      isDead_ = true;
      Logger::Log("EnemyBullet: Intercepted!\n");
      // TODO: ここで爆発エフェクトなどを生成できればなお良い
    }
  }
}

void EnemyBullet::Draw3D() {
  if (!isDead_ && object3d_) {
    object3d_->Draw();
  }
}
