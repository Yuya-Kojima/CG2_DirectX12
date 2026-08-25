#include "BehaviorStraight.h"
#include "Actor/Enemy.h"
#include "Math/MathUtil.h"

void BehaviorStraight::Update(Enemy* enemy) {
    if (!enemy) return;

    if (!isInitialized_) {
        isInitialized_ = true;
        // 最初の一回だけ、進行方向のベクトルを決定して固定する
        // レール基準で「手前（Zマイナス方向）」に飛んでくるということは、当時のレールの -forward 方向
        moveDirection_ = { -enemy->GetBaseForward().x, -enemy->GetBaseForward().y, -enemy->GetBaseForward().z };
        
        // カメラが存在しない場合はEnemy自身のForwardなどを使うフォールバック
        if (!enemy->GetCamera()) {
             Vector3 moveDir = enemy->GetMoveDirection();
             moveDirection_ = { moveDir.x, moveDir.y, moveDir.z };
        }
    }

    // 途中でベクトルを変えず、ただワールド空間で直進し続ける
    float speed = enemy->GetSpeed();
    enemy->GetTransform().translate.x += moveDirection_.x * speed;
    enemy->GetTransform().translate.y += moveDirection_.y * speed;
    enemy->GetTransform().translate.z += moveDirection_.z * speed;
}
