#include "BehaviorFighter.h"
#include "Actor/Enemy.h"
#include "Actor/Player.h"
#include "Math/MathUtil.h"
#include <cmath>

BehaviorFighter::BehaviorFighter() : state_(State::Enter), stateTimer_(0.0f) {}

void BehaviorFighter::Update(Enemy *enemy) {
  if (!enemy)
    return;

  stateTimer_ += 1.0f / 60.0f;

  const Vector3& basePos = enemy->GetBasePosition();
  const Vector3& baseRight = enemy->GetBaseRight();
  const Vector3& baseUp = enemy->GetBaseUp();
  const Vector3& baseForward = enemy->GetBaseForward();

  switch (state_) {
  case State::Enter:
    UpdateEnter(enemy, basePos, baseRight, baseUp, baseForward);
    break;
  case State::Combat:
    UpdateCombat(enemy, basePos, baseRight, baseUp, baseForward);
    break;
  case State::Evade:
    UpdateEvade(enemy, basePos, baseRight, baseUp, baseForward);
    break;
  case State::Retreat:
    UpdateRetreat(enemy, basePos, baseRight, baseUp, baseForward);
    break;
  }
}

void BehaviorFighter::UpdateEnter(Enemy *enemy, const Vector3& cameraPos, const Vector3& cameraRight, const Vector3& cameraUp, const Vector3& cameraForward) {
  const Vector3 &spawnOffset = enemy->GetSpawnOffset();

  // 登場位置（プレイヤーの少し前方に固定）
  enemy->GetTransform().translate =
      cameraPos +
      Vector3{cameraRight.x * spawnOffset.x, cameraRight.y * spawnOffset.x,
              cameraRight.z * spawnOffset.x} +
      Vector3{cameraUp.x * spawnOffset.y, cameraUp.y * spawnOffset.y,
              cameraUp.z * spawnOffset.y} +
      Vector3{cameraForward.x * spawnOffset.z, cameraForward.y * spawnOffset.z,
              cameraForward.z * spawnOffset.z};

  if (stateTimer_ >= 3.0f) {
    state_ = State::Combat;
    stateTimer_ = 0.0f;
  }
}

void BehaviorFighter::UpdateCombat(Enemy *enemy, const Vector3& cameraPos, const Vector3& cameraRight, const Vector3& cameraUp, const Vector3& cameraForward) {
  auto player = enemy->GetPlayer();
  if (!player)
    return;

  const Vector3 &spawnOffset = enemy->GetSpawnOffset();

  // ふわふわ漂う
  float floatOffset = std::sin(stateTimer_ * 2.0f) * 2.0f;

  enemy->GetTransform().translate =
      cameraPos +
      Vector3{cameraRight.x * spawnOffset.x, cameraRight.y * spawnOffset.x,
              cameraRight.z * spawnOffset.x} +
      Vector3{cameraUp.x * (spawnOffset.y + floatOffset),
              cameraUp.y * (spawnOffset.y + floatOffset),
              cameraUp.z * (spawnOffset.y + floatOffset)} +
      Vector3{cameraForward.x * spawnOffset.z, cameraForward.y * spawnOffset.z,
              cameraForward.z * spawnOffset.z};

  // プレイヤーを注視する
  Vector3 playerPos = player->GetTransform().translate;
  Vector3 dirToPlayer = {playerPos.x - enemy->GetTransform().translate.x,
                         playerPos.y - enemy->GetTransform().translate.y,
                         playerPos.z - enemy->GetTransform().translate.z};

  // Y軸回転を計算（atan2）
  enemy->GetTransform().rotate.y = std::atan2(dirToPlayer.x, dirToPlayer.z);

  if (stateTimer_ >= 3.0f) {
    state_ = State::Evade;
    stateTimer_ = 0.0f;
    // 回避方向を決定（左右ランダムに近い形、今回は簡易的に右か左）
    evadeDir_ = (spawnOffset.x > 0)
                    ? cameraRight
                    : Vector3{-cameraRight.x, -cameraRight.y, -cameraRight.z};
  }
}

void BehaviorFighter::UpdateEvade(Enemy *enemy, const Vector3& cameraPos, const Vector3& cameraRight, const Vector3& cameraUp, const Vector3& cameraForward) {
  // 決定した回避方向へ急加速
  enemy->GetTransform().translate.x += evadeDir_.x * 2.0f;
  enemy->GetTransform().translate.y += evadeDir_.y * 2.0f;
  enemy->GetTransform().translate.z += evadeDir_.z * 2.0f;

  if (stateTimer_ >= 1.0f) {
    state_ = State::Retreat;
    stateTimer_ = 0.0f;
  }
}

void BehaviorFighter::UpdateRetreat(Enemy *enemy, const Vector3& cameraPos, const Vector3& cameraRight, const Vector3& cameraUp, const Vector3& cameraForward) {
  // 画面奥（前方）へ飛び去る
  enemy->GetTransform().translate.x += cameraForward.x * 3.0f;
  enemy->GetTransform().translate.y += cameraForward.y * 3.0f;
  enemy->GetTransform().translate.z += cameraForward.z * 3.0f;
}
