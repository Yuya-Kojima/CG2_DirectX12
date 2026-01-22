#pragma once
#include "Math/Vector3.h"

namespace GamePhysics {
// スイープ判定（粗め）: start から delta の移動方向上でスフィアが AABB と衝突するかを判定します。
// 衝突する場合は true を返し、衝突時刻 toi ([0,1]) とおおまかな分離法線 normal を出力します。
bool SweepSphereAabb2D(const Vector3& start, const Vector3& delta, float r,
                       const Vector3& bmin, const Vector3& bmax,
                       float& toi, Vector3& normal);

// 静的解決: スフィアの中心 pos が AABB にめり込んでいる場合、XZ 平面上で外側へ押し出します。
void ResolveSphereAabb2D(Vector3& pos, float r, const Vector3& bmin, const Vector3& bmax);

} // namespace GamePhysics
