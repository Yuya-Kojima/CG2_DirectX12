#pragma once
#include "IEnemyBehavior.h"
#include "Math/Vector3.h"

// 前方へ直進するだけの基本的なAI
class BehaviorStraight : public IEnemyBehavior {
public:
    void Update(Enemy* enemy) override;

private:
    bool isInitialized_ = false;
    Vector3 moveDirection_ = {0.0f, 0.0f, 0.0f};
};
