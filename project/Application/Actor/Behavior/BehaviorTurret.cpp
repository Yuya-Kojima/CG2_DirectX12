#include "BehaviorTurret.h"
#include "Actor/Enemy.h"
#include "Actor/Player.h"
#include "Camera/ICamera.h"
#include "Actor/EnemyBullet.h"
#include "Framework/ActorManager.h"
#include "Framework/PrefabManager.h"
#include <cmath>
#include <algorithm>

BehaviorTurret::BehaviorTurret() : stateTimer_(0.0f), shotTimer_(30) {}

void BehaviorTurret::Update(Enemy* enemy) {
  if (!enemy) return;

  stateTimer_ += 1.0f / 60.0f;
  shotTimer_++;

  auto camera = enemy->GetCamera();
  auto player = enemy->GetPlayer();
  if (!camera || !player) return;

  // 1. スポーン時のワールド座標を固定保持（固定砲台としてカメラ追従を無効化）

  Vector3 playerPos = player->GetTransform().translate;
  Vector3 myPos = enemy->GetTransform().translate;

  // 自機の移動速度ベクトル（前フレームからの差分）を計測
  Vector3 playerVel = {0.0f, 0.0f, 0.0f};
  if (hasPrevPlayerPos_) {
    playerVel = {
        playerPos.x - prevPlayerPos_.x,
        playerPos.y - prevPlayerPos_.y,
        playerPos.z - prevPlayerPos_.z
    };
  }
  prevPlayerPos_ = playerPos;
  hasPrevPlayerPos_ = true;

  // 初回フレーム等で速度が取れていない場合のフォールバック（カメラ前進速度：秒速40m = 毎フレーム約0.67m）
  float velSq = playerVel.x * playerVel.x + playerVel.y * playerVel.y + playerVel.z * playerVel.z;
  if (velSq < 0.01f) {
    const Vector3& camFwd = enemy->GetBaseForward();
    playerVel = {camFwd.x * 0.67f, camFwd.y * 0.67f, camFwd.z * 0.67f};
  }

  // 弾速（毎フレーム1.3m = 秒速約78m）
  float bulletSpeed = 1.3f;

  // 未来予測位置（インターセプト）の物理計算
  // 砲台から現在の自機位置への相対ベクトル d0
  Vector3 d0 = {playerPos.x - myPos.x, playerPos.y - myPos.y, playerPos.z - myPos.z};
  float currentDist = std::sqrt(d0.x * d0.x + d0.y * d0.y + d0.z * d0.z);

  // 2次方程式: (|V|^2 - s^2) * t^2 + 2*(d0 . V) * t + |d0|^2 = 0
  float a = (playerVel.x * playerVel.x + playerVel.y * playerVel.y + playerVel.z * playerVel.z) - (bulletSpeed * bulletSpeed);
  float b = 2.0f * (d0.x * playerVel.x + d0.y * playerVel.y + d0.z * playerVel.z);
  float c = d0.x * d0.x + d0.y * d0.y + d0.z * d0.z;

  float interceptTime = 0.0f;
  float disc = b * b - 4.0f * a * c;
  if (disc >= 0.0f && std::abs(a) > 0.0001f) {
    float sqrtDisc = std::sqrt(disc);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);
    if (t1 > 0.0f && t2 > 0.0f) {
      interceptTime = (std::min)(t1, t2);
    } else if (t1 > 0.0f) {
      interceptTime = t1;
    } else if (t2 > 0.0f) {
      interceptTime = t2;
    }
  }

  // 解が得られなかった場合のフォールバック（相対接近速度による近似）
  if (interceptTime <= 0.0f) {
    interceptTime = currentDist / (bulletSpeed + 0.67f);
  }

  // 未来の自機到達位置
  Vector3 futurePlayerPos = {
      playerPos.x + playerVel.x * interceptTime,
      playerPos.y + playerVel.y * interceptTime,
      playerPos.z + playerVel.z * interceptTime
  };

  // 2. 砲台は未来位置を先読みして注視（旋回・ピッチ）
  Vector3 aimDir = {futurePlayerPos.x - myPos.x, futurePlayerPos.y - myPos.y, futurePlayerPos.z - myPos.z};
  float aimDist = std::sqrt(aimDir.x * aimDir.x + aimDir.y * aimDir.y + aimDir.z * aimDir.z);

  if (aimDist > 0.001f) {
    enemy->GetTransform().rotate.y = std::atan2(aimDir.x, aimDir.z);
    enemy->GetTransform().rotate.x = -std::asin(std::clamp(aimDir.y / aimDist, -1.0f, 1.0f));
  }

  // 3. 射撃処理 (初弾は出現後約1.5秒＝90フレーム、以降は約2.0秒＝120フレーム間隔)
  if (shotTimer_ >= 120) {
    shotTimer_ = 0;

    // すれ違い直前・通過後の理不尽な横殴り・背後射撃を防止するフェア設計
    // (1) 自機より手前45m以上離れていること（秒速40mで約1.1秒以上の迎撃・視認猶予を確保。接近時は撃ちやめ）
    // (2) 砲台自身がプレイヤー画面内（マージン0.1）に視認できていること
    float zDistToPlayer = myPos.z - playerPos.z;
    bool isFairDistance = (zDistToPlayer >= 45.0f) && (currentDist <= 280.0f);
    bool isVisibleOnScreen = enemy->IsInScreen(0.1f);

    if (isFairDistance && isVisibleOnScreen && aimDist > 0.001f) {
      Vector3 bulletVelocity = {
          (aimDir.x / aimDist) * bulletSpeed,
          (aimDir.y / aimDist) * bulletSpeed,
          (aimDir.z / aimDist) * bulletSpeed
      };

      auto bullet = std::make_unique<EnemyBullet>();
      // 未来位置へ一直線に飛ぶ直進光弾（NormalDestructible）として発射
      bullet->Initialize(
          PrefabManager::GetInstance()->GetObject3dRenderer(),
          myPos,
          bulletVelocity,
          const_cast<Player*>(player),
          EnemyBulletType::NormalDestructible
      );
      ActorManager::GetInstance()->AddActor(std::move(bullet));
    }
  }
}
