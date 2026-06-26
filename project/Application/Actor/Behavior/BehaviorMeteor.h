#pragma once
#include "IEnemyBehavior.h"
#include "Math/Vector3.h"

class BehaviorMeteor : public IEnemyBehavior {
public:
  BehaviorMeteor();
  ~BehaviorMeteor() override = default;

  void Update(Enemy* enemy) override;

private:
  enum class State { Wait, Charge };
  State state_ = State::Wait;
  float stateTimer_ = 0.0f;
  Vector3 chargeVelocity_ = {0.0f, 0.0f, 0.0f};
};
