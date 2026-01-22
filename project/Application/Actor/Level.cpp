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
                    { x * tileSize_ + wallThickness*0.5f, wallHeight, minZ_ + wallThickness*0.5f }, false);
        AddWallAABB({ x * tileSize_ - wallThickness*0.5f, 0.0f, maxZ_ - wallThickness*0.5f },
                    { x * tileSize_ + wallThickness*0.5f, wallHeight, maxZ_ + wallThickness*0.5f }, false);
    }
    for (int z = -halfZ; z <= halfZ; ++z) {
        AddWallAABB({ minX_ - wallThickness*0.5f, 0.0f, z * tileSize_ - wallThickness*0.5f },
                    { minX_ + wallThickness*0.5f, wallHeight, z * tileSize_ + wallThickness*0.5f }, false);
        AddWallAABB({ maxX_ - wallThickness*0.5f, 0.0f, z * tileSize_ - wallThickness*0.5f },
                    { maxX_ + wallThickness*0.5f, wallHeight, z * tileSize_ + wallThickness*0.5f }, false);
    }

    // 内部のハザード
    AddHazard({0.0f, 0.0f, 2.0f}, 0.5f);
    AddHazard({-2.0f, 0.0f, -1.5f}, 0.7f);

    BuildWallGrid();
}

void Level::QueryWalls(const Vector3& qmin, const Vector3& qmax, std::vector<const Level::AABB*>& out) const {
    out.clear();
    if (wallGrid_.empty()) {
        for (const auto& a : walls_) {
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
                if (a.max.x < qmin.x || a.min.x > qmax.x) continue;
                if (a.max.z < qmin.z || a.min.z > qmax.z) continue;
                out.push_back(&a);
            }
        }
    }
}

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
    if (bestY != -FLT_MAX) {
        hitY = bestY; return true;
    }
    if (origin.y >= 0.0f && origin.y - maxDist <= 0.0f) {
        hitY = 0.0f; return true;
    }
    return false;
}

void Level::Update(float dt) {
    if (!renderer_) return;
    // 簡易カリング: カメラ位置から遠い床タイルは更新をスキップ
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

void Level::AddHazard(const Vector3& pos, float radius) {
    auto hz = std::make_unique<Hazard>();
    hz->Initialize(renderer_, pos, radius);
    hazards_.push_back(std::move(hz));
}

void Level::KeepInsideBounds(Vector3& pos, float halfSize) const {
    if (pos.x < minX_ + halfSize) pos.x = minX_ + halfSize;
    if (pos.x > maxX_ - halfSize) pos.x = maxX_ - halfSize;
    if (pos.z < minZ_ + halfSize) pos.z = minZ_ + halfSize;
    if (pos.z > maxZ_ - halfSize) pos.z = maxZ_ - halfSize;
}

void Level::AddWallAABB(const Vector3& min, const Vector3& max, bool visual) {
    walls_.push_back({min, max});
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

void Level::ResolveCollision(Vector3& pos, float radius) const {
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
