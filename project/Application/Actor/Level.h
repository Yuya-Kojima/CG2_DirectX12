#pragma once
#include "Math/Vector3.h"
#include <vector>
#include <memory>

class Object3d;
class Object3dRenderer;
class Hazard;

class Level {
public:
    struct AABB { Vector3 min; Vector3 max; };

    // 初期化: Object3dRenderer を渡して床タイルのグリッドを生成
    void Initialize(Object3dRenderer* renderer, int tilesX, int tilesZ, float tileSize);
    void Update(float dt);
    void Draw();

    // XZ 範囲内に位置を保持（半径分のマージン込み）
    void KeepInsideBounds(Vector3& pos, float halfSize) const;

    // 球(中心 pos, 半径) と壁 AABB 群の衝突を解決（XZ 平面）
    void ResolveCollision(Vector3& pos, float radius) const;
    bool IsHazardHit(const Vector3& pos, float radius) const;
    void AddHazard(const Vector3& pos, float radius);

    // 指定 AABB に重なる壁のみ列挙
    void QueryWalls(const Vector3& qmin, const Vector3& qmax, std::vector<const AABB*>& out) const;

    // 下方向にレイキャストして床面や壁の上面を検出
    bool RaycastDown(const Vector3& origin, float maxDist, float& hitY) const;

    float GetMinX() const { return minX_; }
    float GetMaxX() const { return maxX_; }
    float GetMinZ() const { return minZ_; }
    float GetMaxZ() const { return maxZ_; }

    // 壁 AABB の追加
    void AddWallAABB(const Vector3& min, const Vector3& max, bool visual);

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
