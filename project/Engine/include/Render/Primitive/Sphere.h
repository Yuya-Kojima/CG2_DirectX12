#pragma once
#include "Math/MathUtil.h"
#include "Math/VertexData.h"
#include <cstdint>
#include <vector>

class Sphere {
public:
    const std::vector<VertexData>& GetVertices() const { return vertices_; }
    uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices_.size()); }

    uint32_t GetDivide() const { return divide_; }
    float GetRadius() const { return radius_; }

    void Build(uint32_t divide, float radius);

private:
    uint32_t divide_ = 0;
    float radius_ = 1.0f;

    std::vector<VertexData> vertices_;
};
