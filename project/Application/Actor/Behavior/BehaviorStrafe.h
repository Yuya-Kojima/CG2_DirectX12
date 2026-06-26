#pragma once
#include "IEnemyBehavior.h"

class BehaviorStrafe : public IEnemyBehavior {
public:
  BehaviorStrafe();
  ~BehaviorStrafe() override = default;

  void Update(Enemy* enemy) override;

private:
  float stateTimer_ = 0.0f;
  int shotTimer_ = 0;
};
