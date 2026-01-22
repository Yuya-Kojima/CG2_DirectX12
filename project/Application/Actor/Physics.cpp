#include "Actor/Physics.h"
#include "Math/Vector3.h"
#include <algorithm>
#include <cmath>

namespace GamePhysics {

// Clamp: 値を [lo, hi] に丸めるユーティリティ
static inline float Clamp(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }

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
        float pen = r - dist;
        if (dist > 0.0001f) {
            pos.x += (dx / dist) * pen;
            pos.z += (dz / dist) * pen;
        } else {
            float pushL = std::abs(pos.x - bmin.x);
            float pushR = std::abs(bmax.x - pos.x);
            float pushB = std::abs(pos.z - bmin.z);
            float pushF = std::abs(bmax.z - pos.z);
            float m = std::min(std::min(pushL, pushR), std::min(pushB, pushF));
            if (m == pushL) pos.x = bmin.x - r; else if (m == pushR) pos.x = bmax.x + r;
            else if (m == pushB) pos.z = bmin.z - r; else pos.z = bmax.z + r;
        }
    }
}

// スイープ判定（XZ 平面、粗め）
// スイープ判定（XZ 平面、粗め）
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
