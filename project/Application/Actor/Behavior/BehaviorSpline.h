#pragma once
#include "IEnemyBehavior.h"
#include "Math/MathUtil.h"
#include <vector>

// Catmull-Rom曲線に沿って移動するAI
class BehaviorSpline : public IEnemyBehavior {
public:
    // duration: レールを走り切る秒数
    // isWorldSpace: true=ワールド空間として評価, false=カメラからのローカル空間として評価
    BehaviorSpline(const std::vector<Vector3>& waypoints, float duration, bool isWorldSpace = false);
    void Update(Enemy* enemy) override;

private:
    std::vector<Vector3> waypoints_;
    float duration_;
    bool isWorldSpace_;
};
