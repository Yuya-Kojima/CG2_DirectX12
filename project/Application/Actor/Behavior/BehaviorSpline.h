#pragma once
#include "IEnemyBehavior.h"
#include "Math/MathUtil.h"
#include <vector>

// Catmull-Rom曲線に沿って移動するAI
class BehaviorSpline : public IEnemyBehavior {
public:
    BehaviorSpline(const std::vector<Vector3>& waypoints);
    void Update(Enemy* enemy) override;

private:
    std::vector<Vector3> waypoints_;
};
