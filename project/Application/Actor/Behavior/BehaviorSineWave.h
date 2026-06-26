#pragma once
#include "IEnemyBehavior.h"

// 波打ちながら進むAI
class BehaviorSineWave : public IEnemyBehavior {
public:
    void Update(Enemy* enemy) override;
};
