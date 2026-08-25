#include "BehaviorSineWave.h"
#include "Actor/Enemy.h"
#include <cmath>

#include "Math/MathUtil.h"

void BehaviorSineWave::Update(Enemy* enemy) {
    if (!enemy) return;

    if (!isInitialized_) {
        isInitialized_ = true;
        // 最初の一回だけ基準を決定する
        moveDirection_ = { -enemy->GetBaseForward().x, -enemy->GetBaseForward().y, -enemy->GetBaseForward().z };
        rightDirection_ = enemy->GetBaseRight();
        initialPos_ = enemy->GetTransform().translate;
        
        if (!enemy->GetCamera()) {
             Vector3 moveDir = enemy->GetMoveDirection();
             moveDirection_ = { moveDir.x, moveDir.y, moveDir.z };
             rightDirection_ = { 1.0f, 0.0f, 0.0f }; // 仮の右方向
        }
    }

    float aliveTime = enemy->GetAliveTime();
    float speed = enemy->GetSpeed();

    // 基準軌道（ワールド空間で真っ直ぐ進んだ場合の位置）
    float distance = speed * aliveTime * 60.0f; // 60FPS基準の移動距離
    Vector3 basePos = {
        initialPos_.x + moveDirection_.x * distance,
        initialPos_.y + moveDirection_.y * distance,
        initialPos_.z + moveDirection_.z * distance
    };

    // 基準軌道に対して左右（rightDirection）にサイン波で揺らす
    float sineOffset = std::sin(aliveTime * 5.0f) * 20.0f;
    enemy->GetTransform().translate = {
        basePos.x + rightDirection_.x * sineOffset,
        basePos.y + rightDirection_.y * sineOffset,
        basePos.z + rightDirection_.z * sineOffset
    };
}
