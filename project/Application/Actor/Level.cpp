#include "Actor/Level.h"
#include "Actor/Hazard.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
#include "Model/ModelManager.h"
#include "Camera/GameCamera.h"
#include <algorithm>
#include <cmath>
#include <cfloat>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

void Level::Initialize(Object3dRenderer* renderer, int tilesX, int tilesZ, float tileSize) {
    renderer_ = renderer;
    tilesX_ = tilesX; tilesZ_ = tilesZ; tileSize_ = tileSize;
    floorTiles_.clear();
    int halfX = tilesX_ / 2;
    int halfZ = tilesZ_ / 2;
    minX_ = (-halfX) * tileSize_;
    maxX_ = (halfX) * tileSize_;
    minZ_ = (-halfZ) * tileSize_;
    maxZ_ = (halfZ) * tileSize_;

    for (int z = -halfZ; z <= halfZ; ++z) {
        for (int x = -halfX; x <= halfX; ++x) {
            auto obj = std::make_unique<Object3d>();
            obj->Initialize(renderer_);
            ModelManager::GetInstance()->LoadModel("plane.obj");
            obj->SetModel("plane.obj");
            obj->SetScale({tileSize_, 1.0f, tileSize_});
            obj->SetTranslation({x * tileSize_, 0.0f, z * tileSize_});
            floorTiles_.push_back(std::move(obj));
        }
    }

    // 外周の壁AABB（見た目はボックス）高さは固定
    const float wallThickness = tileSize_;
    const float wallHeight = 2.0f;
    for (int x = -halfX; x <= halfX; ++x) {
        AddWallAABB({ x * tileSize_ - wallThickness*0.5f, 0.0f, minZ_ - wallThickness*0.5f },
                    { x * tileSize_ + wallThickness*0.5f, wallHeight, minZ_ + wallThickness*0.5f }, false, 0, 0);
        AddWallAABB({ x * tileSize_ - wallThickness*0.5f, 0.0f, maxZ_ - wallThickness*0.5f },
                    { x * tileSize_ + wallThickness*0.5f, wallHeight, maxZ_ + wallThickness*0.5f }, false, 0, 0);
    }
    for (int z = -halfZ; z <= halfZ; ++z) {
        AddWallAABB({ minX_ - wallThickness*0.5f, 0.0f, z * tileSize_ - wallThickness*0.5f },
                    { minX_ + wallThickness*0.5f, wallHeight, z * tileSize_ + wallThickness*0.5f }, false, 0, 0);
        AddWallAABB({ maxX_ - wallThickness*0.5f, 0.0f, z * tileSize_ - wallThickness*0.5f },
                    { maxX_ + wallThickness*0.5f, wallHeight, z * tileSize_ + wallThickness*0.5f }, false, 0, 0);
    }

    // 内部のハザード
    AddHazard({0.0f, 0.0f, 2.0f}, 0.5f);
    AddHazard({-2.0f, 0.0f, -1.5f}, 0.7f);

    BuildWallGrid();
    BuildNavGrid();
}

// 指定範囲に重なる壁を列挙します。ignoreId が 0 でない場合、その ownerId を持つ壁は除外します。
void Level::QueryWalls(const Vector3& qmin, const Vector3& qmax, std::vector<const Level::AABB*>& out, uint32_t ignoreId) const {
    out.clear();
    if (wallGrid_.empty()) {
        for (const auto& a : walls_) {
            if (ignoreId != 0 && a.ownerId == ignoreId) continue;
            if (a.max.x < qmin.x || a.min.x > qmax.x) continue;
            if (a.max.z < qmin.z || a.min.z > qmax.z) continue;
            out.push_back(&a);
        }
        return;
    }

    int gx0 = (int)std::floor((qmin.x - minX_) / cellSizeX_);
    int gz0 = (int)std::floor((qmin.z - minZ_) / cellSizeZ_);
    int gx1 = (int)std::floor((qmax.x - minX_) / cellSizeX_);
    int gz1 = (int)std::floor((qmax.z - minZ_) / cellSizeZ_);
    gx0 = std::max(0, std::min(gridX_-1, gx0));
    gz0 = std::max(0, std::min(gridZ_-1, gz0));
    gx1 = std::max(0, std::min(gridX_-1, gx1));
    gz1 = std::max(0, std::min(gridZ_-1, gz1));

    for (int gz = gz0; gz <= gz1; ++gz) {
        for (int gx = gx0; gx <= gx1; ++gx) {
            const auto& list = wallGrid_[gz * gridX_ + gx];
            for (int idx : list) {
                const auto& a = walls_[idx];
                if (ignoreId != 0 && a.ownerId == ignoreId) continue;
                if (a.max.x < qmin.x || a.min.x > qmax.x) continue;
                if (a.max.z < qmin.z || a.min.z > qmax.z) continue;
                out.push_back(&a);
            }
        }
    }
}

void Level::QueryWalls(const Vector3& qmin, const Vector3& qmax, std::vector<const Level::AABB*>& out) const {
    QueryWalls(qmin, qmax, out, 0);
}

// ナビゲーショングリッドを構築し、各タイルの通行可否を設定します
void Level::BuildNavGrid() {
    int nx = tilesX_;
    int nz = tilesZ_;
    nav_.Initialize(nx, nz, tileSize_, Vector3{ minX_, 0.0f, minZ_ });

    for (int z = 0; z < nz; ++z) {
        for (int x = 0; x < nx; ++x) {
            Vector3 center = nav_.TileCenter(x, z);
            bool ok = true;
            for (const auto& a : walls_) {
                if (center.x >= a.min.x && center.x <= a.max.x && center.z >= a.min.z && center.z <= a.max.z) { ok = false; break; }
            }
            nav_.SetWalkable(x, z, ok);
        }
    }
}

// 壁の空間分割グリッドを作成してクエリを高速化します
void Level::BuildWallGrid() {
    gridX_ = tilesX_ + 1;
    gridZ_ = tilesZ_ + 1;
    cellSizeX_ = tileSize_;
    cellSizeZ_ = tileSize_;
    wallGrid_.assign(gridX_ * gridZ_, {});
    for (int i = 0; i < (int)walls_.size(); ++i) {
        const auto& a = walls_[i];
        int gx0 = (int)std::floor((a.min.x - minX_) / cellSizeX_);
        int gz0 = (int)std::floor((a.min.z - minZ_) / cellSizeZ_);
        int gx1 = (int)std::floor((a.max.x - minX_) / cellSizeX_);
        int gz1 = (int)std::floor((a.max.z - minZ_) / cellSizeZ_);
        gx0 = std::max(0, std::min(gridX_-1, gx0));
        gz0 = std::max(0, std::min(gridZ_-1, gz0));
        gx1 = std::max(0, std::min(gridX_-1, gx1));
        gz1 = std::max(0, std::min(gridZ_-1, gz1));
        for (int gz = gz0; gz <= gz1; ++gz) {
            for (int gx = gx0; gx <= gx1; ++gx) {
                wallGrid_[gz * gridX_ + gx].push_back(i);
            }
        }
    }
}

// 下方向へのレイキャスト: 指定上端から maxDist の範囲で床や壁の上面を検出します
bool Level::RaycastDown(const Vector3& origin, float maxDist, float& hitY) const {
    float bestY = -FLT_MAX;
    for (const auto& a : walls_) {
        if (origin.x < a.min.x || origin.x > a.max.x) continue;
        if (origin.z < a.min.z || origin.z > a.max.z) continue;
        float topY = a.max.y;
        if (origin.y >= topY && origin.y - maxDist <= topY) {
            if (topY > bestY) bestY = topY;
        }
    }
    if (bestY != -FLT_MAX) { hitY = bestY; return true; }
    if (origin.y >= 0.0f && origin.y - maxDist <= 0.0f) { hitY = 0.0f; return true; }
    return false;
}

// レベル更新: 可視範囲内の床タイルや壁オブジェクトを更新します
void Level::Update(float dt) {
    if (!renderer_) return;
    GameCamera* cam = renderer_->GetDefaultCamera();
    if (!cam) return;
    Vector3 camPos = cam->GetTranslate();
    const float viewRadius2 = 80.0f * 80.0f;
    for (auto& t : floorTiles_) {
        Vector3 c = t->GetTranslation();
        float dx = c.x - camPos.x;
        float dz = c.z - camPos.z;
        if ((dx*dx + dz*dz) > viewRadius2) continue;
        t->Update();
    }
    for (auto& w : wallObjs_) { w->Update(); }
    for (auto& h : hazards_) { h->Update(0.0f); }
}

// レベル描画: 可視範囲内の床・壁・ハザードを描画します
void Level::Draw() {
    GameCamera* cam = renderer_ ? renderer_->GetDefaultCamera() : nullptr;
    Vector3 camPos = cam ? cam->GetTranslate() : Vector3{0,0,0};
    const float viewRadius2 = 80.0f * 80.0f;
    for (auto& t : floorTiles_) {
        Vector3 c = t->GetTranslation();
        float dx = c.x - camPos.x;
        float dz = c.z - camPos.z;
        if ((dx*dx + dz*dz) > viewRadius2) continue;
        t->Draw();
    }
    for (auto& w : wallObjs_) { w->Draw(); }
    for (auto& h : hazards_) { h->Draw(); }
}

// ハザードとの衝突判定（XZ 平面）: pos を中心、radius の球で判定します
bool Level::IsHazardHit(const Vector3& pos, float radius) const {
    for (const auto& h : hazards_) {
        Vector3 hp = h->GetPosition();
        float dx = pos.x - hp.x;
        float dz = pos.z - hp.z;
        float dist2 = dx*dx + dz*dz;
        float rr = radius + h->GetRadius();
        if (dist2 < rr*rr) return true;
    }
    return false;
}

// ハザードを追加してレンダラに登録します
void Level::AddHazard(const Vector3& pos, float radius) {
    auto hz = std::make_unique<Hazard>();
    hz->Initialize(renderer_, pos, radius);
    hazards_.push_back(std::move(hz));
}

// レベルの境界内に位置を収めます（半径 halfSize を考慮）
void Level::KeepInsideBounds(Vector3& pos, float halfSize) const {
    if (pos.x < minX_ + halfSize) pos.x = minX_ + halfSize;
    if (pos.x > maxX_ - halfSize) pos.x = maxX_ - halfSize;
    if (pos.z < minZ_ + halfSize) pos.z = minZ_ + halfSize;
    if (pos.z > maxZ_ - halfSize) pos.z = maxZ_ - halfSize;
}

// 壁 AABB を追加し、visual フラグが立っていれば視覚オブジェクトも作成します
void Level::AddWallAABB(const Vector3& min, const Vector3& max, bool visual, uint32_t ownerLayer, uint32_t ownerId) {
    walls_.push_back({min, max, ownerLayer, ownerId});
    if (visual && renderer_) {
        auto cube = std::make_unique<Object3d>();
        cube->Initialize(renderer_);
        ModelManager::GetInstance()->LoadModel("Cube.obj");
        cube->SetModel("Cube.obj");
        Vector3 size { max.x - min.x, max.y - min.y, max.z - min.z };
        Vector3 center { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f };
        cube->SetScale({ size.x, size.y, size.z });
        cube->SetTranslation(center);
        wallObjs_.push_back(std::move(cube));
    }
}

// 指定の球（pos, radius）について壁との貫通があれば押し出して解決します
// considerCollision が false の場合は何もしません
void Level::ResolveCollision(Vector3& pos, float radius, bool considerCollision) const {
    if (!considerCollision) return;
    for (const auto& aabb : walls_) {
        float cx = std::max(aabb.min.x, std::min(pos.x, aabb.max.x));
        float cz = std::max(aabb.min.z, std::min(pos.z, aabb.max.z));
        float dx = pos.x - cx;
        float dz = pos.z - cz;
        float dist2 = dx*dx + dz*dz;
        if (dist2 < radius*radius) {
            float dist = std::sqrt(std::max(0.0001f, dist2));
            float pen = radius - dist;
            if (dist > 0.0001f) {
                pos.x += (dx / dist) * pen;
                pos.z += (dz / dist) * pen;
            } else {
                float left = pos.x - aabb.min.x;
                float right = aabb.max.x - pos.x;
                float back = pos.z - aabb.min.z;
                float front = aabb.max.z - pos.z;
                float minPen = std::min(std::min(left, right), std::min(back, front));
                if (minPen == left) pos.x = aabb.min.x - radius;
                else if (minPen == right) pos.x = aabb.max.x + radius;
                else if (minPen == back) pos.z = aabb.min.z - radius;
                else pos.z = aabb.max.z + radius;
            }
        }
    }
}

// 指定 id を持つ所有者参照を全てクリアします
void Level::ClearOwnerId(uint32_t id) {
    if (id == 0) return;
    for (auto &a : walls_) {
        if (a.ownerId == id) a.ownerId = 0;
    }
}

// 指定 ownerId を持つ壁を全て削除します（視覚オブジェクトは呼び出し側で管理する）
void Level::RemoveWallsByOwnerId(uint32_t ownerId) {
    if (ownerId == 0) return;
    // remove any wall entries that have this ownerId
    walls_.erase(std::remove_if(walls_.begin(), walls_.end(), [&](const AABB& a){ return a.ownerId == ownerId; }), walls_.end());
    // also remove visual objects that referenced this ownerId: best-effort by matching transforms/positions
    // For simplicity we won't try to correlate object3d instances here; caller should manage visual objects.
    RebuildWallGrid();
}

// 壁空間グリッドを再構築します（Query 用のインデックスを再作成）
void Level::RebuildWallGrid() {
    gridX_ = tilesX_ + 1;
    gridZ_ = tilesZ_ + 1;
    cellSizeX_ = tileSize_;
    cellSizeZ_ = tileSize_;
    wallGrid_.assign(gridX_ * gridZ_, {});
    for (int i = 0; i < (int)walls_.size(); ++i) {
        const auto& a = walls_[i];
        int gx0 = (int)std::floor((a.min.x - minX_) / cellSizeX_);
        int gz0 = (int)std::floor((a.min.z - minZ_) / cellSizeZ_);
        int gx1 = (int)std::floor((a.max.x - minX_) / cellSizeX_);
        int gz1 = (int)std::floor((a.max.z - minZ_) / cellSizeZ_);
        gx0 = std::max(0, std::min(gridX_-1, gx0));
        gz0 = std::max(0, std::min(gridZ_-1, gz0));
        gx1 = std::max(0, std::min(gridX_-1, gx1));
        gz1 = std::max(0, std::min(gridZ_-1, gz1));
        for (int gz = gz0; gz <= gz1; ++gz) {
            for (int gx = gx0; gx <= gx1; ++gx) {
                wallGrid_[gz * gridX_ + gx].push_back(i);
            }
        }
    }
}
