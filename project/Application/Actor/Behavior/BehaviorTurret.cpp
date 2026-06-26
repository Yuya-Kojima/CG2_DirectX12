#include "BehaviorTurret.h"
#include "Actor/Enemy.h"
#include "Actor/Player.h"
#include "Camera/ICamera.h"
#include "Actor/EnemyBullet.h"
#include "Framework/ActorManager.h"
#include "Framework/PrefabManager.h"
#include <cmath>
#include <algorithm>

BehaviorTurret::BehaviorTurret() : stateTimer_(0.0f), shotTimer_(0) {}

void BehaviorTurret::Update(Enemy* enemy) {
  if (!enemy) return;

  stateTimer_ += 1.0f / 60.0f;
  shotTimer_++;

  auto camera = enemy->GetCamera();
  auto player = enemy->GetPlayer();
  if (!camera || !player) return;

  // 1. 位置の更新（カメラ相対で固定位置に留まる）
  Vector3 cameraPos = camera->GetTranslate();
  Vector3 cameraRight = camera->GetRight();
  Vector3 cameraUp = camera->GetUp();
  Vector3 cameraForward = camera->GetForward();
  const Vector3& spawnOffset = enemy->GetSpawnOffset();

  enemy->GetTransform().translate =
      cameraPos +
      Vector3{cameraRight.x * spawnOffset.x, cameraRight.y * spawnOffset.x, cameraRight.z * spawnOffset.x} +
      Vector3{cameraUp.x * spawnOffset.y, cameraUp.y * spawnOffset.y, cameraUp.z * spawnOffset.y} +
      Vector3{cameraForward.x * spawnOffset.z, cameraForward.y * spawnOffset.z, cameraForward.z * spawnOffset.z};

  // 2. プレイヤーの方向を向く
  Vector3 playerPos = player->GetTransform().translate;
  Vector3 myPos = enemy->GetTransform().translate;
  Vector3 dirToPlayer = {
      playerPos.x - myPos.x,
      playerPos.y - myPos.y,
      playerPos.z - myPos.z
  };
  enemy->GetTransform().rotate.y = std::atan2(dirToPlayer.x, dirToPlayer.z);

  // 3. 射撃処理 (120フレーム = 2秒に1回)
  if (shotTimer_ >= 120) {
    shotTimer_ = 0;
    
    float dist = std::sqrt(dirToPlayer.x * dirToPlayer.x + dirToPlayer.y * dirToPlayer.y + dirToPlayer.z * dirToPlayer.z);
    Vector3 bulletVelocity = {0.0f, 0.0f, 0.0f};
    
    if (dist > 0.001f) {
      float bulletSpeed = 2.0f; 
      bulletVelocity = {
          (dirToPlayer.x / dist) * bulletSpeed,
          (dirToPlayer.y / dist) * bulletSpeed,
          (dirToPlayer.z / dist) * bulletSpeed
      };
    }

    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Initialize(PrefabManager::GetInstance()->GetObject3dRenderer(), myPos, bulletVelocity, const_cast<Player*>(player));
    ActorManager::GetInstance()->AddActor(std::move(bullet));
  }
}
