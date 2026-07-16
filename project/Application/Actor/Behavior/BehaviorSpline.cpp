#include "BehaviorSpline.h"
#include "Actor/Enemy.h"
#include <cmath>
#include <algorithm>

BehaviorSpline::BehaviorSpline(const std::vector<Vector3>& waypoints, bool isWorldSpace)
    : waypoints_(waypoints), isWorldSpace_(isWorldSpace) {
}

void BehaviorSpline::Update(Enemy* enemy) {
    if (!enemy || waypoints_.size() < 4) return;

    float aliveTime = enemy->GetAliveTime();
    float speed = enemy->GetSpeed(); // e.g., 0.5f means 0.5 segments per second

    int maxSegments = static_cast<int>(waypoints_.size()) - 3;
    if (maxSegments < 1) return;

    float totalT = aliveTime * speed;
    
    // 終端でストップするか、消滅させるか
    if (totalT >= maxSegments) {
        totalT = static_cast<float>(maxSegments);
        enemy->TakeDamage(9999, true); // 走り終わったら消滅させる
        return;
    }

    int segmentIndex = static_cast<int>(totalT);
    if (segmentIndex >= maxSegments) {
        segmentIndex = maxSegments - 1;
    }

    float t = totalT - segmentIndex;

    const Vector3& p0 = waypoints_[segmentIndex];
    const Vector3& p1 = waypoints_[segmentIndex + 1];
    const Vector3& p2 = waypoints_[segmentIndex + 2];
    const Vector3& p3 = waypoints_[segmentIndex + 3];

    // Spline座標の計算 (MathUtil.hのCatmullRomを使用)
    Vector3 localPos = CatmullRom(p0, p1, p2, p3, t);

    // 向きの計算 (少し先の未来の座標を見てそこを向く)
    float tNext = t + 0.05f;
    Vector3 nextLocalPos;
    if (tNext >= 1.0f && segmentIndex + 1 < maxSegments) {
        nextLocalPos = CatmullRom(waypoints_[segmentIndex+1], waypoints_[segmentIndex+2], waypoints_[segmentIndex+3], waypoints_[segmentIndex+4], tNext - 1.0f);
    } else {
        tNext = std::min(1.0f, tNext);
        nextLocalPos = CatmullRom(p0, p1, p2, p3, tNext);
    }
    
    Vector3 nextWorldPos;

    if (isWorldSpace_) {
        enemy->GetTransform().translate = localPos;
        nextWorldPos = nextLocalPos;
    } else {
        // カメラ相対座標系へ変換
        const Vector3& cameraPos = enemy->GetBasePosition();
        const Vector3& cameraRight = enemy->GetBaseRight();
        const Vector3& cameraUp = enemy->GetBaseUp();
        const Vector3& cameraForward = enemy->GetBaseForward();
        const Vector3& spawnOffset = enemy->GetSpawnOffset();

        // スポーンオフセットも加味する（軌道全体をオフセットぶんずらす）
        Vector3 finalLocal = {
            localPos.x + spawnOffset.x,
            localPos.y + spawnOffset.y,
            localPos.z + spawnOffset.z
        };

        enemy->GetTransform().translate = 
            cameraPos +
            Vector3{cameraRight.x * finalLocal.x, cameraRight.y * finalLocal.x, cameraRight.z * finalLocal.x} +
            Vector3{cameraUp.x * finalLocal.y, cameraUp.y * finalLocal.y, cameraUp.z * finalLocal.y} +
            Vector3{cameraForward.x * finalLocal.z, cameraForward.y * finalLocal.z, cameraForward.z * finalLocal.z};
        
        Vector3 nextFinalLocal = {
            nextLocalPos.x + spawnOffset.x,
            nextLocalPos.y + spawnOffset.y,
            nextLocalPos.z + spawnOffset.z
        };

        nextWorldPos = 
            cameraPos +
            Vector3{cameraRight.x * nextFinalLocal.x, cameraRight.y * nextFinalLocal.x, cameraRight.z * nextFinalLocal.x} +
            Vector3{cameraUp.x * nextFinalLocal.y, cameraUp.y * nextFinalLocal.y, cameraUp.z * nextFinalLocal.y} +
            Vector3{cameraForward.x * nextFinalLocal.z, cameraForward.y * nextFinalLocal.z, cameraForward.z * nextFinalLocal.z};
    }

    Vector3 dir = {
        nextWorldPos.x - enemy->GetTransform().translate.x,
        nextWorldPos.y - enemy->GetTransform().translate.y,
        nextWorldPos.z - enemy->GetTransform().translate.z
    };
    
    if (std::abs(dir.x) > 0.001f || std::abs(dir.z) > 0.001f) {
        enemy->GetTransform().rotate.y = std::atan2(dir.x, dir.z);
    }
}
