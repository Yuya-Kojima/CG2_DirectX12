#pragma once
#include "Math/Vector3.h"
#include <vector>

class Level;

class NavigationGrid {
public:
    // デフォルトコンストラクタ
    NavigationGrid() = default;

    // コンストラクタ: サイズ、タイルの大きさ、原点を指定して初期化
    NavigationGrid(int nx, int nz, float tileSize, const Vector3& origin);

    // 初期化: 指定されたパラメータでグリッドメモリを確保し、全タイルを通行可能に設定
    void Initialize(int nx, int nz, float tileSize, const Vector3& origin);

    // 指定したタイル座標の通行可否を設定
    void SetWalkable(int x, int z, bool ok);

    // 指定したタイル座標が通行可能か確認
    bool IsWalkable(int x, int z) const;

    // タイル座標（インデックス）からワールド座標（タイルの中心点）を算出
    Vector3 TileCenter(int x, int z) const;

    // ワールド座標からタイル座標（インデックス）へ変換。範囲外ならfalseを返す
    bool WorldToTile(const Vector3& world, int& outx, int& outz) const;

    
    //アルゴリズムによる経路探索
    std::vector<Vector3> FindPath(int sx, int sz, int gx, int gz) const;

    // グリッドの横幅（タイル数）を取得
    int GetNX() const { return nx_; }
    // グリッドの奥行き（タイル数）を取得
    int GetNZ() const { return nz_; }

private:
    int nx_ = 0; // X方向のタイル数
    int nz_ = 0; // Z方向のタイル数
    float tileSize_ = 1.0f; // 1タイルの辺の長さ
    Vector3 origin_ { 0, 0, 0 }; // グリッドの起点（最小のX,Zを持つ角の座標）
    std::vector<uint8_t> walkable_; // 各タイルの通行可否フラグ (0:不可, 1:可)
};