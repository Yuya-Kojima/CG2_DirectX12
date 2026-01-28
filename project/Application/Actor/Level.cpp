#include "Actor/Level.h"
#include "Actor/Hazard.h"
#include "Camera/GameCamera.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
#include <algorithm>
#include <cfloat>
#include <cmath>

// Windows.h等で定義されるmin/maxマクロの干渉を防ぐ
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// ---------------------------------------------------------
// 初期化
// ---------------------------------------------------------

void Level::Initialize(Object3dRenderer* renderer, int tilesX, int tilesZ, float tileSize)
{
    renderer_ = renderer;
    tilesX_ = tilesX;
    tilesZ_ = tilesZ;
    tileSize_ = tileSize;
    floorTiles_.clear();

    // 座標範囲の計算（中心を0,0にする）
    int halfX = tilesX_ / 2;
    int halfZ = tilesZ_ / 2;
    minX_ = (-halfX) * tileSize_;
    maxX_ = (halfX)*tileSize_;
    minZ_ = (-halfZ) * tileSize_;
    maxZ_ = (halfZ)*tileSize_;

    // 床タイルの生成
    for (int z = -halfZ; z <= halfZ; ++z) {
        for (int x = -halfX; x <= halfX; ++x) {
            auto obj = std::make_unique<Object3d>();
            obj->Initialize(renderer_);
            // Use Block model for floor tiles so stage shows block-style floor
            ModelManager::GetInstance()->LoadModel("floor.obj");
            obj->SetModel("floor.obj");
            obj->SetScale({ tileSize_, 1.0f, tileSize_ });
            obj->SetTranslation({ x * tileSize_, -1.5f * tileSize_, z * tileSize_ });
            floorTiles_.push_back(std::move(obj));
        }
    }

    // 外周の壁AABBを配置（見えないがプレイヤーを閉じ込める壁）
    const float wallThickness = tileSize_;
    const float wallHeight = 2.0f;
    for (int x = -halfX; x <= halfX; ++x) {
        // 奥の壁
        AddWallAABB({ x * tileSize_ - wallThickness * 0.5f, 0.0f, minZ_ - wallThickness * 0.5f },
            { x * tileSize_ + wallThickness * 0.5f, wallHeight, minZ_ + wallThickness * 0.5f }, false, 0, 0);
        // 手前の壁
        AddWallAABB({ x * tileSize_ - wallThickness * 0.5f, 0.0f, maxZ_ - wallThickness * 0.5f },
            { x * tileSize_ + wallThickness * 0.5f, wallHeight, maxZ_ + wallThickness * 0.5f }, false, 0, 0);
    }
    for (int z = -halfZ; z <= halfZ; ++z) {
        // 左の壁
        AddWallAABB({ minX_ - wallThickness * 0.5f, 0.0f, z * tileSize_ - wallThickness * 0.5f },
            { minX_ + wallThickness * 0.5f, wallHeight, z * tileSize_ + wallThickness * 0.5f }, false, 0, 0);
        // 右の壁
        AddWallAABB({ maxX_ - wallThickness * 0.5f, 0.0f, z * tileSize_ - wallThickness * 0.5f },
            { maxX_ + wallThickness * 0.5f, wallHeight, z * tileSize_ + wallThickness * 0.5f }, false, 0, 0);
    }

    // （開発用のサンプルハザード配置は削除しました）
    // ハザードはステージデータ(JSON)から追加する運用に統一します。

    // 高速化用グリッドとAI用パス検索グリッドを構築
    BuildWallGrid();
    BuildNavGrid();
}

// ---------------------------------------------------------
// クエリ処理
// ---------------------------------------------------------

void Level::QueryWalls(const Vector3& qmin, const Vector3& qmax, std::vector<const Level::AABB*>& out, uint32_t ignoreId) const
{
    out.clear();

    // グリッドが未構築の場合は全線形探索（フォールバック）
    if (wallGrid_.empty()) {
        for (const auto& a : walls_) {
            if (ignoreId != 0 && a.ownerId == ignoreId)
                continue;
            if (a.max.x < qmin.x || a.min.x > qmax.x)
                continue;
            if (a.max.z < qmin.z || a.min.z > qmax.z)
                continue;
            out.push_back(&a);
        }
        return;
    }

    // クエリ範囲をグリッド座標に変換
    int gx0 = (int)std::floor((qmin.x - minX_) / cellSizeX_);
    int gz0 = (int)std::floor((qmin.z - minZ_) / cellSizeZ_);
    int gx1 = (int)std::floor((qmax.x - minX_) / cellSizeX_);
    int gz1 = (int)std::floor((qmax.z - minZ_) / cellSizeZ_);

    // 範囲外をクランプ
    gx0 = std::max(0, std::min(gridX_ - 1, gx0));
    gz0 = std::max(0, std::min(gridZ_ - 1, gz0));
    gx1 = std::max(0, std::min(gridX_ - 1, gx1));
    gz1 = std::max(0, std::min(gridZ_ - 1, gz1));

    // 該当するグリッドセル内の壁のみをチェック
    for (int gz = gz0; gz <= gz1; ++gz) {
        for (int gx = gx0; gx <= gx1; ++gx) {
            const auto& list = wallGrid_[gz * gridX_ + gx];
            for (int idx : list) {
                const auto& a = walls_[idx];
                if (ignoreId != 0 && a.ownerId == ignoreId)
                    continue;
                // AABB同士の重なり判定
                if (a.max.x < qmin.x || a.min.x > qmax.x)
                    continue;
                if (a.max.z < qmin.z || a.min.z > qmax.z)
                    continue;

                // 二重追加を避けるためのチェック（複数のセルに跨る壁対策）
                if (std::find(out.begin(), out.end(), &a) == out.end()) {
                    out.push_back(&a);
                }
            }
        }
    }
}

void Level::QueryWalls(const Vector3& qmin, const Vector3& qmax, std::vector<const Level::AABB*>& out) const
{
    QueryWalls(qmin, qmax, out, 0); // ignoreIdなしで呼び出し
}

void Level::AddWallOBB(const Level::OBB& obb, bool visual)
{
    obbs_.push_back(obb);
    // visual creation omitted
}

void Level::QueryOBBs(const Vector3& qmin, const Vector3& qmax, std::vector<const Level::OBB*>& out) const
{
    out.clear();
    for (const auto& o : obbs_) {
        if (o.center.x + o.halfExtents.x < qmin.x || o.center.x - o.halfExtents.x > qmax.x) continue;
        if (o.center.z + o.halfExtents.z < qmin.z || o.center.z - o.halfExtents.z > qmax.z) continue;
        if (std::find(out.begin(), out.end(), &o) == out.end()) out.push_back(&o);
    }
}

void Level::RemoveOBBsByOwnerId(uint32_t ownerId)
{
    if (ownerId == 0) return;
    obbs_.erase(std::remove_if(obbs_.begin(), obbs_.end(), [&](const OBB& o) { return o.ownerId == ownerId; }), obbs_.end());
}

// ---------------------------------------------------------
// グリッド構築
// ---------------------------------------------------------

void Level::BuildNavGrid()
{
    int nx = tilesX_;
    int nz = tilesZ_;
    nav_.Initialize(nx, nz, tileSize_, Vector3 { minX_, 0.0f, minZ_ });

    // 各タイルが壁と重なっているかチェックし、通行可否を決定
    for (int z = 0; z < nz; ++z) {
        for (int x = 0; x < nx; ++x) {
            Vector3 center = nav_.TileCenter(x, z);
            bool ok = true;
            for (const auto& a : walls_) {
                if (center.x >= a.min.x && center.x <= a.max.x && center.z >= a.min.z && center.z <= a.max.z) {
                    ok = false;
                    break;
                }
            }
            // OBB の簡易判定: XZ平面で中心がOBBのローカルAABBに含まれるかをチェック
            for (const auto& o : obbs_) {
                // 回転を無視したAABBで簡易判定（将来は回転考慮の正確判定に置換可）
                if (center.x >= o.center.x - o.halfExtents.x && center.x <= o.center.x + o.halfExtents.x &&
                    center.z >= o.center.z - o.halfExtents.z && center.z <= o.center.z + o.halfExtents.z) {
                    ok = false;
                    break;
                }
            }
            nav_.SetWalkable(x, z, ok);
        }
    }
}

void Level::BuildWallGrid()
{
    gridX_ = tilesX_ + 1;
    gridZ_ = tilesZ_ + 1;
    cellSizeX_ = tileSize_;
    cellSizeZ_ = tileSize_;
    wallGrid_.assign(gridX_ * gridZ_, {});

    // すべての壁AABBを、重なっているグリッドセルに登録
    for (int i = 0; i < (int)walls_.size(); ++i) {
        const auto& a = walls_[i];
        int gx0 = (int)std::floor((a.min.x - minX_) / cellSizeX_);
        int gz0 = (int)std::floor((a.min.z - minZ_) / cellSizeZ_);
        int gx1 = (int)std::floor((a.max.x - minX_) / cellSizeX_);
        int gz1 = (int)std::floor((a.max.z - minZ_) / cellSizeZ_);

        gx0 = std::max(0, std::min(gridX_ - 1, gx0));
        gz0 = std::max(0, std::min(gridZ_ - 1, gz0));
        gx1 = std::max(0, std::min(gridX_ - 1, gx1));
        gz1 = std::max(0, std::min(gridZ_ - 1, gz1));

        for (int gz = gz0; gz <= gz1; ++gz) {
            for (int gx = gx0; gx <= gx1; ++gx) {
                wallGrid_[gz * gridX_ + gx].push_back(i);
            }
        }
    }
}

// ---------------------------------------------------------
// レイキャスト・物理
// ---------------------------------------------------------

bool Level::RaycastDown(const Vector3& origin, float maxDist, float& hitY) const
{
    float bestY = -FLT_MAX;

    // 足元の壁の上面を探す
    for (const auto& a : walls_) {
        if (origin.x < a.min.x || origin.x > a.max.x)
            continue;
        if (origin.z < a.min.z || origin.z > a.max.z)
            continue;

        float topY = a.max.y;
        if (origin.y >= topY && origin.y - maxDist <= topY) {
            if (topY > bestY)
                bestY = topY;
        }
    }

    if (bestY != -FLT_MAX) {
        hitY = bestY;
        return true;
    }

    // OBB の上面も床候補として扱う（単純に中心のY+半高を上面とする）
    for (const auto& o : obbs_) {
        // XZ平面の回転を無視した簡易判定: 中心のローカルAABB内にあるか
        if (origin.x < o.center.x - o.halfExtents.x || origin.x > o.center.x + o.halfExtents.x) continue;
        if (origin.z < o.center.z - o.halfExtents.z || origin.z > o.center.z + o.halfExtents.z) continue;
        float topY = o.center.y + o.halfExtents.y;
        if (origin.y >= topY && origin.y - maxDist <= topY) {
            if (topY > bestY) bestY = topY;
        }
    }
    if (bestY != -FLT_MAX) {
        hitY = bestY;
        return true;
    }

    // 壁がない場合は地面(Y=0)との判定
    if (origin.y >= 0.0f && origin.y - maxDist <= 0.0f) {
        hitY = 0.0f;
        return true;
    }

    return false;
}

// ---------------------------------------------------------
// 更新・描画
// ---------------------------------------------------------

void Level::Update(float dt)
{
    if (!renderer_)
        return;
    GameCamera* cam = renderer_->GetDefaultCamera();
    if (!cam)
        return;

    Vector3 camPos = cam->GetTranslate();
    const float viewRadius2 = 80.0f * 80.0f; // 描画距離の二乗

    // カメラからの距離が近いタイルのみ更新（最適化）
    for (auto& t : floorTiles_) {
        Vector3 c = t->GetTranslation();
        float dx = c.x - camPos.x;
        float dz = c.z - camPos.z;
        if ((dx * dx + dz * dz) > viewRadius2)
            continue;
        t->Update();
    }

    for (auto& w : wallObjs_) {
        w->Update();
    }
    for (auto& h : hazards_) {
        h->Update(dt);
    }
}

void Level::Draw()
{
    GameCamera* cam = renderer_ ? renderer_->GetDefaultCamera() : nullptr;
    Vector3 camPos = cam ? cam->GetTranslate() : Vector3 { 0, 0, 0 };
    const float viewRadius2 = 80.0f * 80.0f;

    // カメラに近いタイルのみ描画
    for (auto& t : floorTiles_) {
        Vector3 c = t->GetTranslation();
        float dx = c.x - camPos.x;
        float dz = c.z - camPos.z;
        if ((dx * dx + dz * dz) > viewRadius2)
            continue;
        t->Draw();
    }

    for (auto& w : wallObjs_) {
        w->Draw();
    }
    for (auto& h : hazards_) {
        h->Draw();
    }
}

// ---------------------------------------------------------
// 判定・管理ユーティリティ
// ---------------------------------------------------------

bool Level::IsHazardHit(const Vector3& pos, float radius) const
{
    for (const auto& h : hazards_) {
        Vector3 hp = h->GetPosition();
        float dx = pos.x - hp.x;
        float dz = pos.z - hp.z;
        float dist2 = dx * dx + dz * dz;
        float rr = radius + h->GetRadius(); // 半径の合計と比較
        if (dist2 < rr * rr)
            return true;
    }
    return false;
}

void Level::AddHazard(const Vector3& pos, float radius)
{
    auto hz = std::make_unique<Hazard>();
    hz->Initialize(renderer_, pos, radius);
    hazards_.push_back(std::move(hz));
}

void Level::KeepInsideBounds(Vector3& pos, float halfSize) const
{
    // フィールド外に出ないよう座標をクランプ
    if (pos.x < minX_ + halfSize)
        pos.x = minX_ + halfSize;
    if (pos.x > maxX_ - halfSize)
        pos.x = maxX_ - halfSize;
    if (pos.z < minZ_ + halfSize)
        pos.z = minZ_ + halfSize;
    if (pos.z > maxZ_ - halfSize)
        pos.z = maxZ_ - halfSize;
}

void Level::AddWallAABB(const Vector3& min, const Vector3& max, bool visual, uint32_t ownerLayer, uint32_t ownerId, const std::string& visualModel)
{
    walls_.push_back({ min, max, ownerLayer, ownerId });

    // 見た目が必要な場合はCubeを生成
    if (visual && renderer_) {
        auto cube = std::make_unique<Object3d>();
        cube->Initialize(renderer_);
        // Default to Block.obj for wall visuals to match floor/block look
        std::string modelToUse = "Block.obj";
        if (!visualModel.empty()) {
            modelToUse = ModelManager::ResolveModelPath(visualModel);
            ModelManager::GetInstance()->LoadModel(modelToUse);
        } else {
            ModelManager::GetInstance()->LoadModel(modelToUse);
        }
        cube->SetModel(modelToUse);

        Vector3 size { max.x - min.x, max.y - min.y, max.z - min.z };
        Vector3 center { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f };

        cube->SetScale({ size.x, size.y, size.z });
        cube->SetTranslation(center);
        wallObjs_.push_back(std::move(cube));
    }
}

void Level::ResolveCollision(Vector3& pos, float radius, bool considerCollision) const
{
    if (!considerCollision)
        return;

    for (const auto& aabb : walls_) {
        // AABB内の最も球に近い点を求める
        float cx = std::max(aabb.min.x, std::min(pos.x, aabb.max.x));
        float cz = std::max(aabb.min.z, std::min(pos.z, aabb.max.z));
        float dx = pos.x - cx;
        float dz = pos.z - cz;
        float dist2 = dx * dx + dz * dz;

        if (dist2 < radius * radius) {
            float dist = std::sqrt(std::max(0.0001f, dist2));
            float pen = radius - dist; // めり込み量

            if (dist > 0.0001f) {
                // 接点方向に押し出す
                pos.x += (dx / dist) * pen;
                pos.z += (dz / dist) * pen;
            } else {
                // 球の中心がAABB内に完全に重なっている場合、最も近い面へ押し出す
                float left = pos.x - aabb.min.x;
                float right = aabb.max.x - pos.x;
                float back = pos.z - aabb.min.z;
                float front = aabb.max.z - pos.z;
                float minPen = std::min({ left, right, back, front });

                if (minPen == left)
                    pos.x = aabb.min.x - radius;
                else if (minPen == right)
                    pos.x = aabb.max.x + radius;
                else if (minPen == back)
                    pos.z = aabb.min.z - radius;
                else
                    pos.z = aabb.max.z + radius;
            }
        }
    }
}

void Level::ClearOwnerId(uint32_t id)
{
    if (id == 0)
        return;
    for (auto& a : walls_) {
        if (a.ownerId == id)
            a.ownerId = 0;
    }
}

void Level::RemoveWallsByOwnerId(uint32_t ownerId)
{
    if (ownerId == 0)
        return;
    // 条件に一致する要素を削除（Remove-Eraseイディオム）
    walls_.erase(std::remove_if(walls_.begin(), walls_.end(),
                     [&](const AABB& a) { return a.ownerId == ownerId; }),
        walls_.end());

    // 壁の数が変わったためグリッドを再構築
    RebuildWallGrid();
}


void Level::RebuildWallGrid()
{
    BuildWallGrid(); // 既存のロジックで再計算
}