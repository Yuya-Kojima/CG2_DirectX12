#pragma once
#include "Math/Vector3.h"
#include <vector>
#include <memory>
#include "Navigation/NavigationGrid.h"

class Object3d;
class Object3dRenderer;
class Hazard;

class Level {
public:
    struct AABB { Vector3 min; Vector3 max; uint32_t ownerLayer = 0; uint32_t ownerId = 0; };

    // 初期化: Object3dRenderer を渡して床タイルのグリッドを生成
    void Initialize(Object3dRenderer* renderer, int tilesX, int tilesZ, float tileSize);
    void Update(float dt);
    void Draw();

    // XZ 範囲内に位置を保持（半径分のマージン込み）
    void KeepInsideBounds(Vector3& pos, float halfSize) const;

    // 球(中心 pos, 半径) と壁 AABB 群の衝突を解決（XZ 平面）
    // considerCollision: if false, the call is a no-op (used for held objects)
    void ResolveCollision(Vector3& pos, float radius, bool considerCollision = true) const;
    bool IsHazardHit(const Vector3& pos, float radius) const;
    void AddHazard(const Vector3& pos, float radius);

    // 指定範囲に重なる壁 AABB を列挙します。
    void QueryWalls(const Vector3& qmin, const Vector3& qmax, std::vector<const AABB*>& out) const;
    // オプション付き Query: ignoreId が 0 でない場合、その ownerId を持つ壁は無視します
    void QueryWalls(const Vector3& qmin, const Vector3& qmax, std::vector<const AABB*>& out, uint32_t ignoreId) const;

    // 下方向にレイキャストして床面や壁の上面を検出
    bool RaycastDown(const Vector3& origin, float maxDist, float& hitY) const;

    float GetMinX() const { return minX_; }
    float GetMaxX() const { return maxX_; }
    float GetMinZ() const { return minZ_; }
    float GetMaxZ() const { return maxZ_; }

    // 壁 AABB の追加
    // ownerLayer: 所有者を示すレイヤーマスク (0 = none)
    // ownerId: 所有者の一意な ID (0 = none)
    void AddWallAABB(const Vector3& min, const Vector3& max, bool visual, uint32_t ownerLayer = 0, uint32_t ownerId = 0);
    // clear ownerId from any AABB that references the given id
    void ClearOwnerId(uint32_t id);
    // remove all wall AABBs that were registered for the given owner id
    void RemoveWallsByOwnerId(uint32_t ownerId);
    // rebuild internal wall grid after dynamic wall changes
    void RebuildWallGrid();
    // Navigation grid
    void BuildNavGrid();
    const NavigationGrid& GetNavGrid() const { return nav_; }
    NavigationGrid nav_;

private:
    Object3dRenderer* renderer_ = nullptr;
    std::vector<std::unique_ptr<Object3d>> floorTiles_;
    std::vector<std::unique_ptr<Object3d>> wallObjs_;
    std::vector<AABB> walls_;
    std::vector<std::unique_ptr<Hazard>> hazards_;
    int tilesX_ = 0;
    int tilesZ_ = 0;
    float tileSize_ = 1.0f;
    float minX_ = 0.0f, maxX_ = 0.0f;
    float minZ_ = 0.0f, maxZ_ = 0.0f;

    // 壁グリッドによる簡易加速
    int gridX_ = 0, gridZ_ = 0;
    float cellSizeX_ = 1.0f, cellSizeZ_ = 1.0f;
    std::vector<std::vector<int>> wallGrid_;
    void BuildWallGrid();
};
