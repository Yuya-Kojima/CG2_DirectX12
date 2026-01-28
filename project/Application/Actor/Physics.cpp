#include "Actor/Physics.h"
#include "Math/Vector3.h"
#include <algorithm>
#include <cmath>

// Simple 2D rotate (around Y) for XZ plane
static inline Vector3 RotateY(const Vector3& v, float yaw) {
    float c = std::cos(yaw);
    float s = std::sin(yaw);
    return Vector3{ v.x * c + v.z * s, v.y, -v.x * s + v.z * c };
}



static inline Vector3 RotateYInv(const Vector3& v, float yaw) {
    // inverse rotation equals rotation by -yaw
    float c = std::cos(yaw);
    float s = std::sin(yaw);
    return Vector3{ v.x * c - v.z * s, v.y, v.x * s + v.z * c };
}

namespace GamePhysics {

// Clamp: 値を [lo, hi] に丸めるユーティリティ
static inline float Clamp(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }

// OBB 対応関数の実装（XZ 平面）
void ResolveSphereObb2D(Vector3& pos, float r, const Vector3& obbCenter, const Vector3& halfExtents, float yaw) {
    // translate into obb local
    Vector3 local = Subtract(pos, obbCenter);
    // rotate into local (inverse rotation)
    local = RotateYInv(local, yaw);

    // treat as AABB with extents halfExtents.x/ z
    Vector3 bmin{ -halfExtents.x, 0.0f, -halfExtents.z };
    Vector3 bmax{ halfExtents.x, 0.0f, halfExtents.z };

    // clamp in local XZ
    auto clampf = [](float v, float a, float b) { return std::max(a, std::min(b, v)); };
    float cx = clampf(local.x, bmin.x, bmax.x);
    float cz = clampf(local.z, bmin.z, bmax.z);
    float dx = local.x - cx;
    float dz = local.z - cz;
    float d2 = dx*dx + dz*dz;
    float rr = r*r;
    if (d2 < rr) {
        float dist = std::sqrt(std::max(0.0001f, d2));
        float pen = r - dist + 0.001f;
        if (dist > 0.0001f) {
            local.x += (dx / dist) * pen;
            local.z += (dz / dist) * pen;
        } else {
            // push out along smallest penetration to face
            float pushL = std::abs(local.x - bmin.x);
            float pushR = std::abs(bmax.x - local.x);
            float pushB = std::abs(local.z - bmin.z);
            float pushF = std::abs(bmax.z - local.z);
            float m = std::min(std::min(pushL, pushR), std::min(pushB, pushF));
            if (m == pushL) local.x = bmin.x - r;
            else if (m == pushR) local.x = bmax.x + r;
            else if (m == pushB) local.z = bmin.z - r;
            else local.z = bmax.z + r;
        }
        // rotate back and write to world
        Vector3 worldLocal = RotateY(local, yaw);
        pos = Add(obbCenter, worldLocal);
    }
}



bool SweepSphereObb2D(const Vector3& start, const Vector3& delta, float r,
                      const Vector3& obbCenter, const Vector3& halfExtents, float yaw,
                      float& toi, Vector3& normal) {
    // move into obb local space
    Vector3 localStart = Subtract(start, obbCenter);
    localStart = RotateYInv(localStart, yaw);
    Vector3 localDelta = RotateYInv(delta, yaw); // rotate delta into local

    // expanded AABB
    Vector3 emin{ -halfExtents.x - r, 0.0f, -halfExtents.z - r };
    Vector3 emax{ halfExtents.x + r, 0.0f, halfExtents.z + r };

    float t0 = 0.0f, t1 = 1.0f;
    normal = {0,0,0};
    if (std::abs(localDelta.x) < 1e-6f) {
        if (localStart.x < emin.x || localStart.x > emax.x) return false;
    } else {
        float inv = 1.0f / localDelta.x;
        float tmin = (emin.x - localStart.x) * inv;
        float tmax = (emax.x - localStart.x) * inv;
        if (tmin > tmax) std::swap(tmin, tmax);
        if (tmin > t0) { t0 = tmin; normal = { (inv < 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f }; }
        if (tmax < t1) t1 = tmax;
        if (t0 > t1) return false;
    }

    if (std::abs(localDelta.z) < 1e-6f) {
        if (localStart.z < emin.z || localStart.z > emax.z) return false;
    } else {
        float inv = 1.0f / localDelta.z;
        float tmin = (emin.z - localStart.z) * inv;
        float tmax = (emax.z - localStart.z) * inv;
        if (tmin > tmax) std::swap(tmin, tmax);
        if (tmin > t0) { t0 = tmin; normal = { 0.0f, 0.0f, (inv < 0.0f) ? 1.0f : -1.0f }; }
        if (tmax < t1) t1 = tmax;
        if (t0 > t1) return false;
    }

    toi = t0;
    if (toi < 0.0f || toi > 1.0f) return false;

    // normal is in local space; rotate it back to world
    Vector3 worldNormal = RotateY(normal, yaw);
    normal = worldNormal;
    return true;
}

// スフィアを AABB から押し出す（XZ 平面）
void ResolveSphereAabb2D(Vector3& pos, float r, const Vector3& bmin, const Vector3& bmax) {
    float cx = Clamp(pos.x, bmin.x, bmax.x);
    float cz = Clamp(pos.z, bmin.z, bmax.z);
    float dx = pos.x - cx;
    float dz = pos.z - cz;
    float d2 = dx*dx + dz*dz;
    float rr = r*r;
    if (d2 < rr) {
        float dist = std::sqrt(std::max(0.0001f, d2));
        // 隙間を確保するために少し余分に押し出す（極小のめり込みが繰り返されるのを防ぐ）
        float pen = r - dist + 0.001f;
        if (dist > 0.0001f) {
            pos.x += (dx / dist) * pen;
            pos.z += (dz / dist) * pen;
        } else {
            float pushL = std::abs(pos.x - bmin.x);
            float pushR = std::abs(bmax.x - pos.x);
            float pushB = std::abs(pos.z - bmin.z);
            float pushF = std::abs(bmax.z - pos.z);
            float m = std::min(std::min(pushL, pushR), std::min(pushB, pushF));
            if (m == pushL) pos.x = bmin.x - r;
            else if (m == pushR) pos.x = bmax.x + r;
            else if (m == pushB) pos.z = bmin.z - r;
            else pos.z = bmax.z + r;
        }
    }
}

// overload with enabled flag
void ResolveSphereAabb2D(Vector3& pos, float r, const Vector3& bmin, const Vector3& bmax, bool collisionEnabled) {
    if (!collisionEnabled) return;
    ResolveSphereAabb2D(pos, r, bmin, bmax);
}

// スイープ判定（XZ 平面、粗め）:
// start から delta の移動区間で球と AABB の交差範囲をパラメトリックに評価します。
// 成功した場合は toi を衝突時刻 (0..1) に設定し、法線の方向を返します。
bool SweepSphereAabb2D(const Vector3& start, const Vector3& delta, float r,
                        const Vector3& bmin, const Vector3& bmax,
                        float& toi, Vector3& normal) {
    Vector3 emin { bmin.x - r, 0.0f, bmin.z - r };
    Vector3 emax { bmax.x + r, 0.0f, bmax.z + r };

    float t0 = 0.0f, t1 = 1.0f;
    normal = {0,0,0};
    if (std::abs(delta.x) < 1e-6f) {
        if (start.x < emin.x || start.x > emax.x) return false;
    } else {
        float inv = 1.0f / delta.x;
        float tmin = (emin.x - start.x) * inv;
        float tmax = (emax.x - start.x) * inv;
        if (tmin > tmax) std::swap(tmin, tmax);
        if (tmin > t0) { t0 = tmin; normal = { (inv < 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f }; }
        if (tmax < t1) t1 = tmax;
        if (t0 > t1) return false;
    }

    if (std::abs(delta.z) < 1e-6f) {
        if (start.z < emin.z || start.z > emax.z) return false;
    } else {
        float inv = 1.0f / delta.z;
        float tmin = (emin.z - start.z) * inv;
        float tmax = (emax.z - start.z) * inv;
        if (tmin > tmax) std::swap(tmin, tmax);
        if (tmin > t0) { t0 = tmin; normal = { 0.0f, 0.0f, (inv < 0.0f) ? 1.0f : -1.0f }; }
        if (tmax < t1) t1 = tmax;
        if (t0 > t1) return false;
    }

    toi = t0;
    return toi >= 0.0f && toi <= 1.0f;
}

} // namespace GamePhysics
