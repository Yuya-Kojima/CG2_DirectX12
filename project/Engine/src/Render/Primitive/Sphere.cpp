#include "Render/Primitive/Sphere.h"
#include <cmath>
#include <numbers>

void Sphere::Build(uint32_t divide, float radius) {
    divide_ = divide;
    radius_ = radius;
    vertices_.clear();

    const float pi = std::numbers::pi_v<float>;
    uint32_t latDivide = divide; // 緯度分割数
    uint32_t lonDivide = divide; // 経度分割数

    // 頂点を生成
    for (uint32_t latIndex = 0; latIndex < latDivide; ++latIndex) {
        float lat = -pi / 2.0f + pi * latIndex / latDivide;
        float nextLat = -pi / 2.0f + pi * (latIndex + 1) / latDivide;
        
        for (uint32_t lonIndex = 0; lonIndex < lonDivide; ++lonIndex) {
            float lon = 2.0f * pi * lonIndex / lonDivide;
            float nextLon = 2.0f * pi * (lonIndex + 1) / lonDivide;

            // 4つの頂点を計算
            Vector4 p0, p1, p2, p3;
            p0 = { std::cos(lat) * std::cos(lon) * radius, std::sin(lat) * radius, std::cos(lat) * std::sin(lon) * radius, 1.0f };
            p1 = { std::cos(nextLat) * std::cos(lon) * radius, std::sin(nextLat) * radius, std::cos(nextLat) * std::sin(lon) * radius, 1.0f };
            p2 = { std::cos(lat) * std::cos(nextLon) * radius, std::sin(lat) * radius, std::cos(lat) * std::sin(nextLon) * radius, 1.0f };
            p3 = { std::cos(nextLat) * std::cos(nextLon) * radius, std::sin(nextLat) * radius, std::cos(nextLat) * std::sin(nextLon) * radius, 1.0f };

            // 法線
            Vector3 n0 = Normalize({p0.x, p0.y, p0.z});
            Vector3 n1 = Normalize({p1.x, p1.y, p1.z});
            Vector3 n2 = Normalize({p2.x, p2.y, p2.z});
            Vector3 n3 = Normalize({p3.x, p3.y, p3.z});

            // UV
            Vector2 uv0 = { static_cast<float>(lonIndex) / lonDivide, 1.0f - static_cast<float>(latIndex) / latDivide };
            Vector2 uv1 = { static_cast<float>(lonIndex) / lonDivide, 1.0f - static_cast<float>(latIndex + 1) / latDivide };
            Vector2 uv2 = { static_cast<float>(lonIndex + 1) / lonDivide, 1.0f - static_cast<float>(latIndex) / latDivide };
            Vector2 uv3 = { static_cast<float>(lonIndex + 1) / lonDivide, 1.0f - static_cast<float>(latIndex + 1) / latDivide };

            // 三角形1
            vertices_.push_back({ p0, uv0, n0 });
            vertices_.push_back({ p1, uv1, n1 });
            vertices_.push_back({ p2, uv2, n2 });

            // 三角形2
            vertices_.push_back({ p1, uv1, n1 });
            vertices_.push_back({ p3, uv3, n3 });
            vertices_.push_back({ p2, uv2, n2 });
        }
    }
}
