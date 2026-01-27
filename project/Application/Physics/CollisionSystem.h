#pragma once
#include "Math/Vector3.h"
#include "Actor/Level.h"
#include <vector>

class Level;

namespace CollisionMask {
    enum : uint32_t { None=0, Player = 1<<0, NPC = 1<<1, Item = 1<<2, Hazard = 1<<3 };
}

struct QueryParams {
    Vector3 start;
    Vector3 delta;
    float radius = 0.5f;
    uint32_t mask = 0xFFFFFFFF;
    bool doSweep = true;
    bool doResolve = true;
    bool doRaycastDown = false;
    // どのレベル対象をテストするか
    enum Targets : uint32_t { TargetWall = 1<<0, TargetHazard = 1<<1 };
    uint32_t targets = TargetWall | TargetHazard;
    // when resolving with level, whether to consider level collisions (useful to skip for held items)
    bool considerLevelCollision = true;
    float raycastMaxDist = 2.0f;
    // choose one: provide ignoreId (preferred) or ignorePtr (legacy)
    uint32_t ignoreId = 0;
    const void* ignorePtr = nullptr;
};

struct Hit {
    const Level::AABB* wall = nullptr;
    float toi = 1.0f;
    Vector3 normal{0,0,0};
    Hit() = default;
    Hit(const Level::AABB* w, float t, const Vector3& n) : wall(w), toi(t), normal(n) {}
};

struct QueryResult {
    bool anyHit = false;
    std::vector<Hit> hits;
    bool hazardHit = false;
    bool groundHit = false;
    float groundY = 0.0f;
};

class CollisionSystem {
public:
    // クエリ実行:
    // QueryParams に基づき、レベルに対してレイキャスト/スイープ/ハザード判定/衝突解決を行い
    // QueryResult を返します。
    // - p.doRaycastDown が true であれば下向きレイを実行して groundY を返す
    // - p.targets に TargetHazard が含まれるとハザード判定を行う
    // - p.doSweep が true なら壁に対するスイープを行い hits を埋める
    // - p.doResolve が true なら最終位置に対してレベルとの衝突解決を行う
    static QueryResult Query(const QueryParams& p, Level& level);
};

