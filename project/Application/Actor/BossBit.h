#pragma once
#include "Actor/Enemy.h"
#include "Math/Vector3.h"

class Boss;

// ボスの弱点コアを覆う物理的な装甲板。
// 全て破壊されるまでボス本体へのダメージを防ぐ役割を持つ。
class BossBit : public Enemy {
public:
    BossBit();
    ~BossBit() override;

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
};
