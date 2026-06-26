#pragma once
#include "IEnemyBehavior.h"

// 前方へ直進するだけの基本的なAI
class BehaviorStraight : public IEnemyBehavior {
public:
    void Update(Enemy* enemy) override;
};
