#pragma once
#include "IEnemyBehavior.h"
#include "Math/Vector3.h"

class BehaviorTurret : public IEnemyBehavior {
public:
  BehaviorTurret();
  ~BehaviorTurret() override = default;

  void Update(Enemy* enemy) override;

private:
  float stateTimer_ = 0.0f;
  int shotTimer_ = 0;
  Vector3 prevPlayerPos_ = {0.0f, 0.0f, 0.0f};
  bool hasPrevPlayerPos_ = false;
};
