#pragma once
#include "IEnemyBehavior.h"

// 座標を一切動かさず、指定された位置（カメラからの相対位置）に留まり続けるAI
class BehaviorStationary : public IEnemyBehavior {
public:
    void Update(Enemy* enemy) override;
};
