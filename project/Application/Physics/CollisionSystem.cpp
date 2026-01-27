#include "Physics/CollisionSystem.h"
#include "Actor/Level.h"
#include "Actor/Physics.h"

QueryResult CollisionSystem::Query(const QueryParams& p, Level& level) {
    QueryResult res;

    // Raycast down if requested
    // 下向きレイキャスト: 足元やプラットフォーム検出用
    if (p.doRaycastDown) {
        float hitY;
        if (level.RaycastDown(p.start, p.raycastMaxDist, hitY)) {
            res.groundHit = true;
            res.groundY = hitY;
        }
    }

    // Hazard check
    // ハザード判定: 移動先の位置でハザードとの重なりをチェック
    if ((p.targets & QueryParams::TargetHazard) != 0) {
        if (level.IsHazardHit(Vector3{p.start.x + p.delta.x, p.start.y + p.delta.y, p.start.z + p.delta.z}, p.radius)) {
            res.hazardHit = true;
        }
    }

    // Wall sweep
    // 壁に対するスイープ判定: start->end を含む範囲の AABB を問い合わせ、各候補に対して
    // スイープ（球 vs AABB）を行い衝突時刻と法線を収集する
    if (p.doSweep) {
        Vector3 a = p.start;
        Vector3 b = Vector3{p.start.x + p.delta.x, p.start.y + p.delta.y, p.start.z + p.delta.z};
        Vector3 qmin { std::min(a.x,b.x) - p.radius, 0.0f, std::min(a.z,b.z) - p.radius };
        Vector3 qmax { std::max(a.x,b.x) + p.radius, 0.0f, std::max(a.z,b.z) + p.radius };
        std::vector<const Level::AABB*> candidates;
        // ignoreId があればそれを渡し、当該所有者の壁を除外
        if (p.ignoreId != 0) level.QueryWalls(qmin, qmax, candidates, p.ignoreId);
        else level.QueryWalls(qmin, qmax, candidates, 0);
        float bestToi = 1.0f;
        for (const auto* aabb : candidates) {
            // マスクで対象外のレイヤーならスキップ
            if (aabb->ownerLayer != 0 && ( (aabb->ownerLayer & p.mask) == 0 )) continue;
            float toi; Vector3 normal;
            if (GamePhysics::SweepSphereAabb2D(p.start, p.delta, p.radius, aabb->min, aabb->max, toi, normal)) {
                Hit h; h.wall = aabb; h.toi = toi; h.normal = normal;
                res.hits.push_back(h);
                res.anyHit = true;
                if (toi < bestToi) bestToi = toi;
            }
        }
    }

    // Optionally resolve
    if (p.doResolve) {
        Vector3 pos = Vector3{p.start.x + p.delta.x, p.start.y + p.delta.y, p.start.z + p.delta.z};
        level.ResolveCollision(pos, p.radius, p.considerLevelCollision);
    }

    return res;
}
