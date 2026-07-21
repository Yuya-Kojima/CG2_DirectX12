#include "EnemyBullet.h"
#include "Render/Object3d/Object3d.h"
#include "Math/MathUtil.h"
#include "Actor/Player.h"
#include "Actor/NormalBullet.h"
#include "Actor/HomingBullet.h"
#include "Collision/SphereCollider.h"
#include "Collision/CollisionManager.h"
#include "Collision/CollisionConfig.h"
#include "Debug/Logger.h"
#include <cmath>

EnemyBullet::EnemyBullet() {}
EnemyBullet::~EnemyBullet() {
  if (collider_) {
    CollisionManager::GetInstance()->Remove(collider_.get());
  }
}

void EnemyBullet::Initialize(Object3dRenderer* renderer, const Vector3& startPos, const Vector3& velocity, Player* player, EnemyBulletType type) {
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
    // ロックオンで壊す大型ミサイル
    object3d_->SetScale({2.5f, 2.5f, 2.5f});
    object3d_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤
    hp_ = 3;
    SetTag("LockOnTarget");
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
  collider_->SetMask(kCollisionAttributePlayer | kCollisionAttributePlayerBullet);
  collider_->SetVelocity(velocity_);
  CollisionManager::GetInstance()->Register(collider_.get());
}

void EnemyBullet::Update() {
  if (isDead_) return;

  lifeTimer_--;
  if (lifeTimer_ <= 0) {
    isDead_ = true;
  }

  // 描画限界距離（カリング距離）による消滅判定
  if (player_ && !player_->IsDead()) {
    Vector3 currentPos = object3d_->GetTranslation();
    Vector3 playerPos = player_->GetTransform().translate;
    Vector3 toPlayer = {playerPos.x - currentPos.x, playerPos.y - currentPos.y, playerPos.z - currentPos.z};
    float distSq = toPlayer.x*toPlayer.x + toPlayer.y*toPlayer.y + toPlayer.z*toPlayer.z;
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
      
      Vector3 toPlayer = {playerPos.x - currentPos.x, playerPos.y - currentPos.y, playerPos.z - currentPos.z};
      float dist = std::sqrt(toPlayer.x*toPlayer.x + toPlayer.y*toPlayer.y + toPlayer.z*toPlayer.z);
      
      if (dist > 0.001f) {
        toPlayer.x /= dist; toPlayer.y /= dist; toPlayer.z /= dist;
      }

      if (aliveFrames_ < 80) {
        // 最初の80フレーム（約1.3秒）は緩やかな空気抵抗で遠くまで広がる
        velocity_.x *= 0.92f;
        velocity_.y *= 0.92f;
        velocity_.z *= 0.92f;
      } else {
        // 以降はプレイヤーに向かって徐々に誘導を開始する
        homingStrength_ += 0.002f;
        if (homingStrength_ > 0.05f) homingStrength_ = 0.05f;
        
        float missileSpeed = 0.35f; // 通常弾(0.5f)よりさらに遅く、ゆっくり迫る
        Vector3 desiredVelocity = {toPlayer.x * missileSpeed, toPlayer.y * missileSpeed, toPlayer.z * missileSpeed};
        
        velocity_.x = Lerp(velocity_.x, desiredVelocity.x, homingStrength_);
        velocity_.y = Lerp(velocity_.y, desiredVelocity.y, homingStrength_);
        velocity_.z = Lerp(velocity_.z, desiredVelocity.z, homingStrength_);
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

  // プレイヤーへの手動当たり判定
  if (player_ && !player_->IsDead()) {
    Vector3 playerPos = player_->GetTransform().translate;
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
    float bulletRadius = 1.5f * object3d_->GetScale().x;
    Vector3 playerScale = player_->GetTransform().scale;
    float maxScale = (std::max)(playerScale.x, (std::max)(playerScale.y, playerScale.z));
    float playerRadius = 1.5f * maxScale;
    
    if (dist < bulletRadius + playerRadius) {
      isDead_ = true; 
      player_->TakeDamage(1);
    }
  }

  object3d_->Update();
}

void EnemyBullet::OnCollision(Collider* other) {
  if (isDead_) return;

  // 破壊不可弾は何が当たっても壊れない
  if (type_ == EnemyBulletType::Indestructible) return;

  // 相手がプレイヤーの弾かチェック
  if (other->GetAttribute() & kCollisionAttributePlayerBullet) {
    BaseActor* bulletOwner = other->GetOwner();
    
    // 相手が通常ショット（NormalBullet）の場合
    if (dynamic_cast<NormalBullet*>(bulletOwner)) {
      if (type_ == EnemyBulletType::NormalDestructible || type_ == EnemyBulletType::LockOnDestructible) {
        hp_ -= 1; // 通常ショットで1ダメージ
        Logger::Log("EnemyBullet: Hit by NormalBullet!\n");
      }
    }
    // 相手がロックオンレーザー（HomingBullet）の場合
    else if (dynamic_cast<HomingBullet*>(bulletOwner)) {
      if (type_ == EnemyBulletType::NormalDestructible || type_ == EnemyBulletType::LockOnDestructible) {
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
