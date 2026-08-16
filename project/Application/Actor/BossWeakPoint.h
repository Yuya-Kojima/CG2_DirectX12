#pragma once
#include "Actor/Enemy.h"
#include "Math/Vector3.h"

class Boss;

// ボスの突進時のみ出現する一時的な弱点（的）。
// これを全て破壊するとカウンター成立となる。
class BossWeakPoint : public Enemy {
public:
    BossWeakPoint();
    ~BossWeakPoint() override;

    void Initialize() override;
    void Update() override;

    // 親となるボスを設定
    void SetBoss(Boss* boss) { boss_ = boss; }
    // ボス中心からの相対配置オフセットを設定
    void SetOffset(const Vector3& offset);

private:
    Boss* boss_ = nullptr;
    Vector3 offset_ = {0.0f, 0.0f, 0.0f};
    Vector3 baseOffset_ = {0.0f, 0.0f, 0.0f};
    float bobbingTimer_ = 0.0f; // フワフワ動かすためのタイマー
};
