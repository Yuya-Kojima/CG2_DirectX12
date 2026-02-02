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

//=========================
// OBB (XZ平面) support
//=========================
// OBB は中心、半幅（extent）、Y回転角(yaw) で定義される。XZ平面での球との当たり判定を行う。
// 静的押し出し（めり込み解消）
void ResolveSphereObb2D(Vector3& pos, float r, const Vector3& obbCenter, const Vector3& halfExtents, float yaw);
// オプション版（有効フラグ）
void ResolveSphereObb2D(Vector3& pos, float r, const Vector3& obbCenter, const Vector3& halfExtents, float yaw, bool collisionEnabled);

// スイープ判定（XZ 平面）: start から delta の移動で衝突するか。衝突時刻 toi と法線 normal を返す。
bool SweepSphereObb2D(const Vector3& start, const Vector3& delta, float r,
                      const Vector3& obbCenter, const Vector3& halfExtents, float yaw,
                      float& toi, Vector3& normal);

// =========================
// AABB (XZ平面) support
// =========================
// AABB は中心と半幅で与える（center, halfExtents）

// 静的押し出し（AABB vs OBB）
void ResolveAabbObb2D(Vector3& aabbCenter, const Vector3& aabbHalfExtents,
                      const Vector3& obbCenter, const Vector3& obbHalfExtents, float yaw);

// 有効フラグ付きオーバーロード
void ResolveAabbObb2D(Vector3& aabbCenter, const Vector3& aabbHalfExtents,
                      const Vector3& obbCenter, const Vector3& obbHalfExtents, float yaw, bool collisionEnabled);

// スイープ判定（AABB が移動）: startCenter から delta の移動で衝突するか。
// 成功した場合は toi と法線を返す。両矩形は XZ 平面で評価。
bool SweepAabbObb2D(const Vector3& startCenter, const Vector3& delta, const Vector3& aabbHalfExtents,
                    const Vector3& obbCenter, const Vector3& obbHalfExtents, float yaw,
                    float& toi, Vector3& normal);

// 瞬間判定: AABB と OBB が重なっているか（XZ平面）
bool AabbIntersectsObb2D(const Vector3& aabbCenter, const Vector3& aabbHalfExtents,
                         const Vector3& obbCenter, const Vector3& obbHalfExtents, float yaw);

// AABB vs AABB (static push-out)
void ResolveAabbAabb2D(Vector3& aabbCenter, const Vector3& aabbHalfExtents,
                      const Vector3& otherCenter, const Vector3& otherHalfExtents);

// AABB sweep vs AABB (moving aabbCenter by delta)
bool SweepAabbAabb2D(const Vector3& startCenter, const Vector3& delta, const Vector3& aabbHalfExtents,
                     const Vector3& otherCenter, const Vector3& otherHalfExtents,
                     float& toi, Vector3& normal);

} // namespace GamePhysics
