#pragma once
#include "IEnemyBehavior.h"

class BehaviorTurret : public IEnemyBehavior {
public:
  BehaviorTurret();
  ~BehaviorTurret() override = default;

  void Update(Enemy* enemy) override;

private:
  float stateTimer_ = 0.0f;
  int shotTimer_ = 0;
};
