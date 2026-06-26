#include "BehaviorSineWave.h"
#include "Actor/Enemy.h"
#include "Camera/ICamera.h"
#include <cmath>

#include "Math/MathUtil.h"

void BehaviorSineWave::Update(Enemy* enemy) {
    if (!enemy) return;

    auto camera = enemy->GetCamera();
    if (camera) {
        Vector3 cameraPos = camera->GetTranslate();
        Vector3 cameraRight = camera->GetRight();
        Vector3 cameraUp = camera->GetUp();
        Vector3 cameraForward = camera->GetForward();

        float aliveTime = enemy->GetAliveTime();
        float speed = enemy->GetSpeed();
        const Vector3& spawnOffset = enemy->GetSpawnOffset();

        float currentXOffset = spawnOffset.x;
        float currentYOffset = spawnOffset.y;
        float currentZOffset = spawnOffset.z;

        // 前方に進みつつ、サイン波で左右に揺れる
        currentZOffset -= speed * aliveTime * 60.0f;
        currentXOffset += std::sin(aliveTime * 5.0f) * 20.0f;

        enemy->GetTransform().translate =
            cameraPos +
            Vector3{cameraRight.x * currentXOffset, cameraRight.y * currentXOffset, cameraRight.z * currentXOffset} +
            Vector3{cameraUp.x * currentYOffset, cameraUp.y * currentYOffset, cameraUp.z * currentYOffset} +
            Vector3{cameraForward.x * currentZOffset, cameraForward.y * currentZOffset, cameraForward.z * currentZOffset};
    } else {
        Vector3 moveDir = enemy->GetMoveDirection();
        float speed = enemy->GetSpeed();
        enemy->GetTransform().translate.x += moveDir.x * speed;
        enemy->GetTransform().translate.y += moveDir.y * speed;
        enemy->GetTransform().translate.z += moveDir.z * speed;
    }
}
