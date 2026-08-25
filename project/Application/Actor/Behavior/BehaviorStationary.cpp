#include "BehaviorStationary.h"
#include "Actor/Enemy.h"
#include "Math/MathUtil.h"

void BehaviorStationary::Update(Enemy* enemy) {
    if (!enemy) return;

    auto camera = enemy->GetCamera();
    if (camera) {
        // Enemy本体が保持している基準ベクトルを取得
        const Vector3& cameraPos = enemy->GetBasePosition();
        const Vector3& cameraRight = enemy->GetBaseRight();
        const Vector3& cameraUp = enemy->GetBaseUp();
        const Vector3& cameraForward = enemy->GetBaseForward();

        const Vector3& spawnOffset = enemy->GetSpawnOffset();

        float currentXOffset = spawnOffset.x;
        float currentYOffset = spawnOffset.y;
        float currentZOffset = spawnOffset.z;

        // ---------------------------------------------------------
        // 【修正点】
        // 毎フレーム cameraPos を基準に計算し直してしまうと、
        // カメラが前進するのに合わせて敵も前進（画面に張り付いた状態）してしまいます。
        // ワールド空間上に完全に留まらせるため、ここでは一切座標を更新しません。
        // スポーン時に一度だけ設定された座標（worldPos）を維持し続けます。
        // ---------------------------------------------------------
    } else {
        // カメラがない場合のフォールバック（ワールド座標での静止、何もしない）
    }
}
