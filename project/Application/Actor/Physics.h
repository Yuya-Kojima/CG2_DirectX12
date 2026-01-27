#pragma once
#include "Math/Vector3.h"

namespace GamePhysics {
// スイープ判定（XZ 平面、簡易版）:
// 指定した開始位置 `start` から `delta` だけ移動すると仮定したときに、
// 半径 `r` の球が AABB (`bmin`/`bmax`) と衝突するかを判定します。
// 衝突する場合は true を返し、衝突時刻 `toi` (0..1) と大まかな分離法線 `normal` を返します。
bool SweepSphereAabb2D(const Vector3& start, const Vector3& delta, float r,
                       const Vector3& bmin, const Vector3& bmax,
                       float& toi, Vector3& normal);

// 静的解決（XZ 平面）:
// 球の中心 `pos` が AABB にめり込んでいる場合、外側へ押し出して貫通を解消します。
void ResolveSphereAabb2D(Vector3& pos, float r, const Vector3& bmin, const Vector3& bmax);
// オプション版: `collisionEnabled` が false の場合は何もしません。
void ResolveSphereAabb2D(Vector3& pos, float r, const Vector3& bmin, const Vector3& bmax, bool collisionEnabled);

} // namespace GamePhysics
