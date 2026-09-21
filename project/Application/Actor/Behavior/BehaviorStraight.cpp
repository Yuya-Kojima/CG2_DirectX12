#include "BehaviorStraight.h"
#include "Actor/Enemy.h"
#include "Math/MathUtil.h"
#include <algorithm>

void BehaviorStraight::Update(Enemy* enemy) {
    if (!enemy) return;

    float aliveTime = enemy->GetAliveTime();
    float speed = enemy->GetSpeed();
    float flySpeed = speed * 1.25f; // 前進速度を適度に引き上げ、疾走感を出す

    // 最初の4.5秒間は、カメラのレール進行方向（カーブ）に追従しながら前方を編隊飛行
    if (aliveTime < 4.5f) {
        if (enemy->GetCamera()) {
            moveDirection_ = enemy->GetBaseForward();
        } else if (!isInitialized_) {
            Vector3 moveDir = enemy->GetMoveDirection();
            moveDirection_ = { moveDir.x, moveDir.y, moveDir.z };
        }
        isInitialized_ = true;

        // レールの向きに合わせてモデルのY軸回転を合わせる
        enemy->GetTransform().rotate.y = std::atan2(moveDirection_.x, moveDirection_.z);

        // 飛行中の姿勢変化を表現する微小ロール角（サイン波）の付加
        float swayRoll = std::sin(aliveTime * 3.0f) * 0.05f;
        enemy->GetTransform().rotate.z = swayRoll;
        enemy->GetTransform().rotate.x = 0.0f;

        // 前方への直線巡航（プレイヤーのロックオン操作猶予時間）
        enemy->GetTransform().translate.x += moveDirection_.x * flySpeed;
        enemy->GetTransform().translate.y += moveDirection_.y * flySpeed;
        enemy->GetTransform().translate.z += moveDirection_.z * flySpeed;
    } 
    // 4.5秒経過時: 未撃破の敵の上昇離脱処理（上方向への加速とピッチ回転）
    else {
        float escapeRatio = (std::min)((aliveTime - 4.5f) / 1.5f, 1.0f); // 0.0 -> 1.0
        float smoothRatio = escapeRatio * escapeRatio; // 2乗イージング（0.0から滑らかに加速）

        float boostSpeed = flySpeed * (1.0f + smoothRatio * 2.5f); // 前進速度も加速

        // カメラの上方向ベクトル（画面の真上）を取得
        Vector3 upDir = enemy->GetBaseUp();
        if (!enemy->GetCamera()) {
            upDir = { 0.0f, 1.0f, 0.0f };
        }

        // イージング適用による急上昇速度の算出
        float climbSpeed = smoothRatio * 3.0f;

        // 前進しながら画面上空へ急上昇
        enemy->GetTransform().translate.x += moveDirection_.x * boostSpeed + upDir.x * climbSpeed;
        enemy->GetTransform().translate.y += moveDirection_.y * boostSpeed + upDir.y * climbSpeed;
        enemy->GetTransform().translate.z += moveDirection_.z * boostSpeed + upDir.z * climbSpeed;

        // 上昇ベクトルに応じたピッチ角（X軸回転）の制御
        enemy->GetTransform().rotate.x = -0.6f * smoothRatio;
        enemy->GetTransform().rotate.z = 0.0f;

        // 完全に画面外（上枠外）へ飛び出したら消滅（画面内では絶対に消滅させない）
        if (aliveTime > 4.8f && !enemy->IsInScreen(30.0f)) {
            enemy->Destroy();
        }
        // 保険（8秒経過で安全に消去）
        else if (aliveTime > 8.0f) {
            enemy->Destroy();
        }
    }
}
