#include "BehaviorSineWave.h"
#include "Actor/Enemy.h"
#include "Math/MathUtil.h"
#include <algorithm>
#include <cmath>

void BehaviorSineWave::Update(Enemy* enemy) {
    if (!enemy) return;

    float aliveTime = enemy->GetAliveTime();
    float speed = enemy->GetSpeed();
    float flySpeed = speed * 1.15f;

    if (!isInitialized_) {
        isInitialized_ = true;
        moveDirection_ = enemy->GetBaseForward();
        rightDirection_ = enemy->GetBaseRight();
        if (!enemy->GetCamera()) {
            Vector3 moveDir = enemy->GetMoveDirection();
            moveDirection_ = { moveDir.x, moveDir.y, moveDir.z };
            rightDirection_ = { 1.0f, 0.0f, 0.0f };
        }
    }

    // カメラの最新基準方向を取得（レールのカーブに自然に追従）
    if (enemy->GetCamera()) {
        moveDirection_ = enemy->GetBaseForward();
        rightDirection_ = enemy->GetBaseRight();
    }

    // 出現位置のXオフセットに基づいて横断方向を決定（左から出たら右へ、右から出たら左へ横断）
    float crossDir = (enemy->GetSpawnOffset().x < 0.0f) ? 1.0f : -1.0f;

    // 敵ごとの配置に基づくわずかな時間差（先頭機から順に0.36秒ずつ追従遅延）
    float rankIndex = (std::max)(0.0f, (std::abs(enemy->GetSpawnOffset().x) - 130.0f) / 10.0f);
    float phaseDelay = rankIndex * 0.36f;
    float t = (std::max)(0.0f, aliveTime - phaseDelay);

    // 画面外から出現し、画面中央を横断して反対側へ抜ける水平滑空移動
    float lateralSpeed = crossDir * 0.85f;

    // コサイン波による上下方向の周期運動（振幅 0.18m/frame）
    Vector3 upDir = enemy->GetBaseUp();
    float waveSpeed = std::cos(t * 1.4f) * 0.18f;

    // 前進 ＋ 横断 ＋ 上下うねりの合成移動
    enemy->GetTransform().translate.x += moveDirection_.x * flySpeed + rightDirection_.x * lateralSpeed + upDir.x * waveSpeed;
    enemy->GetTransform().translate.y += moveDirection_.y * flySpeed + rightDirection_.y * lateralSpeed + upDir.y * waveSpeed;
    enemy->GetTransform().translate.z += moveDirection_.z * flySpeed + rightDirection_.z * lateralSpeed + upDir.z * waveSpeed;

    // 進行方向ベクトルに合わせて機首（Y回転）をスムーズに向ける
    Vector3 totalMoveDir = {
        moveDirection_.x * flySpeed + rightDirection_.x * lateralSpeed,
        moveDirection_.y * flySpeed + rightDirection_.y * lateralSpeed + upDir.y * waveSpeed,
        moveDirection_.z * flySpeed + rightDirection_.z * lateralSpeed
    };
    enemy->GetTransform().rotate.y = std::atan2(totalMoveDir.x, totalMoveDir.z);

    // 横断方向および波の位相に応じたロール角（Z軸回転）の適用
    float targetRoll = -crossDir * 0.35f + std::sin(t * 1.4f) * 0.15f;
    enemy->GetTransform().rotate.z = targetRoll;
    enemy->GetTransform().rotate.x = -waveSpeed * 0.5f; // 上下移動に連動したピッチ角

    // カメラから見た左右位置（カメラのRightベクトルへの射影）
    Vector3 diff = {
        enemy->GetTransform().translate.x - enemy->GetBasePosition().x,
        enemy->GetTransform().translate.y - enemy->GetBasePosition().y,
        enemy->GetTransform().translate.z - enemy->GetBasePosition().z
    };
    float localX = diff.x * rightDirection_.x + diff.y * rightDirection_.y + diff.z * rightDirection_.z;

    // 左から来た敵（crossDir > 0）は画面右側（localX > 50.0f）で画面外へ抜けきったら消滅
    // 右から来た敵（crossDir < 0）は画面左側（localX < -50.0f）で画面外へ抜けきったら消滅
    bool hasCrossedCenter = (crossDir > 0.0f) ? (localX > 50.0f) : (localX < -50.0f);
    if (hasCrossedCenter && !enemy->IsInScreen(50.0f)) {
        enemy->Destroy();
    }
    // 万が一の保険（12秒経過で安全に消去）
    else if (aliveTime > 12.0f) {
        enemy->Destroy();
    }
}
