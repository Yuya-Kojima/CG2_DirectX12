#include "Navigation/NavigationGrid.h"
#include <cmath>
#include <queue>

NavigationGrid::NavigationGrid(int nx, int nz, float tileSize, const Vector3& origin)
{
    Initialize(nx, nz, tileSize, origin);
}

void NavigationGrid::Initialize(int nx, int nz, float tileSize, const Vector3& origin)
{
    // グリッドの基本情報を保存
    nx_ = nx;
    nz_ = nz;
    tileSize_ = tileSize;
    origin_ = origin;

    // 1次元配列で全タイルの通行フラグを管理。初期状態はすべて「通行可(1)」
    walkable_.assign(nx_ * nz_, 1);
}

void NavigationGrid::SetWalkable(int x, int z, bool ok)
{
    // 範囲外チェック
    if (x < 0 || x >= nx_ || z < 0 || z >= nz_)
        return;
    // 座標を1次元インデックスに変換して保存
    walkable_[z * nx_ + x] = ok ? 1 : 0;
}

bool NavigationGrid::IsWalkable(int x, int z) const
{
    // 範囲外は通行不可として扱う
    if (x < 0 || x >= nx_ || z < 0 || z >= nz_)
        return false;
    return walkable_[z * nx_ + x] != 0;
}

Vector3 NavigationGrid::TileCenter(int x, int z) const
{
    float half = tileSize_ * 0.5f;
    // タイルの左下角(origin)から中心点までの距離を計算
    float wx = origin_.x + x * tileSize_ + half;
    float wz = origin_.z + z * tileSize_ + half;
    return Vector3 { wx, origin_.y, wz }; // 高さは原点のY座標を継承
}

bool NavigationGrid::WorldToTile(const Vector3& world, int& outx, int& outz) const
{
    // ワールド座標から原点を引き、タイルサイズで割ってインデックスを算出
    float fx = (world.x - origin_.x) / tileSize_;
    float fz = (world.z - origin_.z) / tileSize_;

    // 切り捨てにより整数座標（タイル座標）へ
    int ix = static_cast<int>(std::floor(fx));
    int iz = static_cast<int>(std::floor(fz));

    // グリッドの範囲内かチェック
    if (ix < 0 || ix >= nx_ || iz < 0 || iz >= nz_)
        return false;

    outx = ix;
    outz = iz;
    return true;
}

std::vector<Vector3> NavigationGrid::FindPath(int sx, int sz, int gx, int gz) const
{
    std::vector<Vector3> empty;
    // 開始点または終了点が通行不可なら即座にリターン
    if (!IsWalkable(sx, sz) || !IsWalkable(gx, gz))
        return empty;

    // A*のノード管理（未使用だが構造的な意図を保持）
    struct Node {
        int x, y;
        float f, g;
    };

    // ヒューリスティック関数（目標までの推定コスト）：ユークリッド距離を使用
    auto h = [&](int x, int y) {
        float dx = float(x - gx);
        float dy = float(y - gz);
        return std::sqrt(dx * dx + dy * dy);
    };

    // 各タイルの最小移動コストを保持。初期値は無限大
    std::vector<float> gscore(nx_ * nz_, 1e30f);
    // 経路を復元するための「親ノード」記録用
    std::vector<int> cameFrom(nx_ * nz_, -1);

    // 優先度付きキュー（Priority Queue）用の要素定義
    struct PQItem {
        float f;
        int idx;
    };
    // F値（実際のコストG + 推定コストH）が小さい順に取り出すための比較関数
    struct Cmp {
        bool operator()(PQItem a, PQItem b) const { return a.f > b.f; }
    };
    std::priority_queue<PQItem, std::vector<PQItem>, Cmp> pq;

    // スタート地点の設定
    int startIdx = sz * nx_ + sx;
    int goalIdx = gz * nx_ + gx;
    gscore[startIdx] = 0.0f;
    pq.push({ h(sx, sz), startIdx });

    // 隣接する4方向（右、左、上、下）の定義
    const int dirs[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

    // --- メイン探索ループ ---
    while (!pq.empty()) {
        PQItem it = pq.top();
        pq.pop();

        int idx = it.idx;
        int cx = idx % nx_; // 現在のタイルX
        int cz = idx / nx_; // 現在のタイルZ

        // ゴールに到達したらループ終了
        if (idx == goalIdx)
            break;

        // 隣接する4方向をチェック
        for (int di = 0; di < 4; ++di) {
            int nx = cx + dirs[di][0];
            int nz = cz + dirs[di][1];

            // 通行可能かチェック
            if (!IsWalkable(nx, nz))
                continue;

            int nidx = nz * this->nx_ + nx;
            // 隣接タイルへの移動コスト（一律 1.0）を加算
            float tentative = gscore[idx] + 1.0f;

            // これまでに発見したルートより低コストなら更新
            if (tentative < gscore[nidx]) {
                gscore[nidx] = tentative;
                cameFrom[nidx] = idx; // どこから来たかを記録
                // F値 = 実績コスト(G) + 目標までの推定コスト(H)
                pq.push({ tentative + h(nx, nz), nidx });
            }
        }
    }

    // ゴールに辿り着けなかった場合
    if (cameFrom[goalIdx] == -1)
        return empty;

    // --- 経路の復元 ---
    std::vector<int> rev;
    int cur = goalIdx;
    while (cur != -1) {
        rev.push_back(cur);
        cur = cameFrom[cur]; // 親を辿ってスタートまで戻る
    }

    // 逆順（ゴール→スタート）になっているので、反転しながらワールド座標に変換
    std::vector<Vector3> path;
    for (int i = (int)rev.size() - 1; i >= 0; --i) {
        int idx = rev[i];
        int tx = idx % nx_;
        int tz = idx / nx_;
        path.push_back(TileCenter(tx, tz));
    }

    return path;
}