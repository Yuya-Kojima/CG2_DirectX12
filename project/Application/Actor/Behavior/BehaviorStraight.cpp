#include "BehaviorStraight.h"
#include "Actor/Enemy.h"
#include "Camera/ICamera.h"
#include "Camera/RailCamera.h"

#include "Math/MathUtil.h"

void BehaviorStraight::Update(Enemy* enemy) {
    if (!enemy) return;

    auto camera = enemy->GetCamera();
    if (camera) {
        // レールカメラ基準の移動
        Vector3 cameraPos = camera->GetTranslate(); 
        Vector3 cameraRight = camera->GetRight();
        Vector3 cameraUp = camera->GetUp();
        Vector3 cameraForward = camera->GetForward();

        // 敵は「カメラの首振り」ではなく「レール自体の位置・向き」を基準に配置する
        if (auto railCam = dynamic_cast<const RailCamera*>(camera)) {
            cameraPos = railCam->GetRailPosition();
            cameraRight = railCam->GetRailRight();
            cameraUp = railCam->GetRailUp();
            cameraForward = railCam->GetRailForward();
        }

        float aliveTime = enemy->GetAliveTime();
        float speed = enemy->GetSpeed();
        const Vector3& spawnOffset = enemy->GetSpawnOffset();

        float currentXOffset = spawnOffset.x;
        float currentYOffset = spawnOffset.y;
        float currentZOffset = spawnOffset.z;

        // まっすぐ手前（Zマイナス方向）に近づいてくる
        currentZOffset -= speed * aliveTime * 60.0f;

        enemy->GetTransform().translate =
            cameraPos +
            Vector3{cameraRight.x * currentXOffset, cameraRight.y * currentXOffset, cameraRight.z * currentXOffset} +
            Vector3{cameraUp.x * currentYOffset, cameraUp.y * currentYOffset, cameraUp.z * currentYOffset} +
            Vector3{cameraForward.x * currentZOffset, cameraForward.y * currentZOffset, cameraForward.z * currentZOffset};
    } else {
        // カメラがない場合のフォールバック（ワールド座標での直進）
        Vector3 moveDir = enemy->GetMoveDirection();
        float speed = enemy->GetSpeed();
        enemy->GetTransform().translate.x += moveDir.x * speed;
        enemy->GetTransform().translate.y += moveDir.y * speed;
        enemy->GetTransform().translate.z += moveDir.z * speed;
    }
}
