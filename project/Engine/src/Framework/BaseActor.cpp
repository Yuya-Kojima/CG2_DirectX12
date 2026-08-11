#include "Framework/BaseActor.h"

BaseActor::BaseActor() {
    // 初期値の設定
    transform_.scale = {1.0f, 1.0f, 1.0f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    transform_.translate = {0.0f, 0.0f, 0.0f};
}

Vector3 BaseActor::CalculateVelocityForCollision() {
    if (!hasInitializedPreviousPos_) {
        previousPos_ = transform_.translate; // 1フレーム目は移動量ゼロにする
        hasInitializedPreviousPos_ = true;
    }
    Vector3 vel = {transform_.translate.x - previousPos_.x,
                   transform_.translate.y - previousPos_.y,
                   transform_.translate.z - previousPos_.z};
    previousPos_ = transform_.translate;
    return vel;
}
