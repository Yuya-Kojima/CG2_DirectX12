#pragma once
#include "Math/Vector3.h"
#include "Navigation/NavigationGrid.h"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

// 前方宣言
class Object3d;
class Object3dRenderer;
class Hazard;

/// <summary>
/// ゲームフィールド（レベル）を管理するクラス。
/// 床タイルの生成、壁のAABB衝突判定、ハザードの配置、ナビグリッドの構築を担当します。
/// </summary>
class Level {
public:
    /// <summary> 軸に平行な境界ボックス（AABB）構造体 </summary>
    struct AABB {
        Vector3 min; // 最小座標 (左下奥)
        Vector3 max; // 最大座標 (右上前)
        uint32_t ownerLayer = 0; // 所有者のレイヤー
        uint32_t ownerId = 0; // 所有者の個別ID
    };

    /// <summary> 回転を持つ境界ボックス（XZ平面での回転のみを扱う） </summary>
    struct OBB {
        Vector3 center;       // 中心座標
        Vector3 halfExtents;  // 半幅 (x, y, z)
        float yaw = 0.0f;     // Y軸回りの回転角（ラジアン）
        uint32_t ownerLayer = 0;
        uint32_t ownerId = 0;
    };

    // ---------------------------------------------------------
    // 基本システムメソッド
    // ---------------------------------------------------------

    /// <summary>
    /// レベルの初期化。床タイルの生成と外周の壁を構築します。
    /// </summary>
    void Initialize(Object3dRenderer* renderer, int tilesX, int tilesZ, float tileSize);

    /// <summary>
    /// タイル配列に基づいて床タイルを生成する。
    /// tiles が空なら全グリッドに床を生成する。
    /// 配列は行優先 (row-major) で z*tilesX + x の順で格納される想定。
    /// 値が 0 のマップチップの位置に床を表示する仕様。
    /// </summary>
    void CreateFloorTiles(const std::vector<int>& tiles = std::vector<int>());

    /// <summary> 毎フレームの更新（可視範囲のタイルやハザードの更新） </summary>
    void Update(float dt);

    /// <summary> レベル内の全オブジェクトを描画 </summary>
    void Draw();

    // ---------------------------------------------------------
    // 衝突判定・境界チェック
    // ---------------------------------------------------------

    /// <summary> 指定した位置をレベルのXZ境界内に収めます（押し出し） </summary>
    void KeepInsideBounds(Vector3& pos, float halfSize) const;

    /// <summary> 球体と壁の衝突を解決し、位置を補正します </summary>
    void ResolveCollision(Vector3& pos, float radius, bool considerCollision = true) const;

    /// <summary> ハザード（危険物）に触れているか判定します </summary>
    bool IsHazardHit(const Vector3& pos, float radius) const;

    /// <summary> 新しいハザードを追加します </summary>
    void AddHazard(const Vector3& pos, float radius);
    /// <summary> 新しいハザードを追加します（表示用モデルを明示的に指定） </summary>
    void AddHazard(const Vector3& pos, float radius, const std::string& model);

    /// <summary> 回転付きOBBを追加します（視覚表示は省略可能） </summary>
    void AddWallOBB(const OBB& obb, bool visual = false);

    // ---------------------------------------------------------
    // クエリ・空間分割
    // ---------------------------------------------------------

    /// <summary> 指定範囲内にある壁のAABBリストを取得します </summary>
    void QueryWalls(const Vector3& qmin, const Vector3& qmax, std::vector<const AABB*>& out) const;

    /// <summary> 指定範囲内にある回転付きOBBのリストを取得します（XZ平面回転のみ） </summary>
    void QueryOBBs(const Vector3& qmin, const Vector3& qmax, std::vector<const OBB*>& out) const;

    /// <summary> 特定のIDを無視して、範囲内の壁AABBリストを取得します </summary>
    void QueryWalls(const Vector3& qmin, const Vector3& qmax, std::vector<const AABB*>& out, uint32_t ignoreId) const;

    /// <summary> 下方向へレイを飛ばし、床や壁の天面の高さを取得します </summary>
    bool RaycastDown(const Vector3& origin, float maxDist, float& hitY) const;

    // ---------------------------------------------------------
    // アクセッサ
    // ---------------------------------------------------------

    float GetMinX() const { return minX_; }
    float GetMaxX() const { return maxX_; }
    float GetMinZ() const { return minZ_; }
    float GetMaxZ() const { return maxZ_; }

    // ---------------------------------------------------------
    // 動的な壁の管理
    // ---------------------------------------------------------

    /// <summary> 壁AABBを追加します。visualがtrueならCubeモデルも生成します。
    /// visualModelが指定されていればそのモデルを表示用に使用します。 </summary>
    void AddWallAABB(const Vector3& min, const Vector3& max, bool visual, uint32_t ownerLayer = 0, uint32_t ownerId = 0, const std::string& visualModel = "");

    /// <summary> 指定IDを持つ壁の所有権を解除します </summary>
    void ClearOwnerId(uint32_t id);

    /// <summary> 指定IDに関連付けられた壁をすべて削除します </summary>
    void RemoveWallsByOwnerId(uint32_t ownerId);
    /// <summary> 指定IDに関連付けられたOBBをすべて削除します </summary>
    void RemoveOBBsByOwnerId(uint32_t ownerId);

    /// <summary> 指定IDに対応する OBB の中心/半幅/yaw を更新します。存在しなければ追加します。 </summary>
    void UpdateOBB(uint32_t ownerId, const OBB& obb);

    /// <summary> 空間分割グリッドを再構築します（動的な壁変更後に呼び出し） </summary>
    void RebuildWallGrid();

    // ---------------------------------------------------------
    // ナビゲーション
    // ---------------------------------------------------------

    /// <summary> AIの移動経路計算用のナビグリッドを構築します </summary>
    void BuildNavGrid();

    /// <summary> ナビグリッドへの参照を取得します </summary>
    const NavigationGrid& GetNavGrid() const { return nav_; }

    NavigationGrid nav_; // 公開されているナビグリッド実体

    // デバッグ用: 登録されている OBB の一覧を取得
    const std::vector<OBB>& GetOBBs() const { return obbs_; }
    // デバッグ/ユーティリティ: 登録されている軸整列AABB一覧を取得
    const std::vector<AABB>& GetWalls() const { return walls_; }

private:
    // --- 描画・アセット関連 ---
    Object3dRenderer* renderer_ = nullptr; // レンダラへの参照
    std::vector<std::unique_ptr<Object3d>> floorTiles_; // 床タイルモデル群
    std::vector<std::unique_ptr<Object3d>> wallObjs_; // 壁の視覚モデル群
    std::vector<AABB> walls_; // 衝突判定用AABB群
    std::vector<OBB> obbs_; // 衝突判定用OBB群（回転あり）
    std::vector<std::unique_ptr<Hazard>> hazards_; // 危険オブジェクト群

    // --- レベル形状パラメータ ---
    int tilesX_ = 0; // X方向のタイル数
    int tilesZ_ = 0; // Z方向のタイル数
    float tileSize_ = 1.0f; // タイル1枚のサイズ
    float minX_ = 0.0f, maxX_ = 0.0f; // レベルのX端
    float minZ_ = 0.0f, maxZ_ = 0.0f; // レベルのZ端

    // --- 空間分割（衝突検知の高速化） ---
    int gridX_ = 0, gridZ_ = 0; // 分割グリッド数
    float cellSizeX_ = 1.0f, cellSizeZ_ = 1.0f; // 1セル辺りのサイズ
    std::vector<std::vector<int>> wallGrid_; // セルごとの壁インデックスリスト

    /// <summary> 内部用：空間分割グリッドの構築メソッド </summary>
    void BuildWallGrid();
};