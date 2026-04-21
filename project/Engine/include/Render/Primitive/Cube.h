#pragma once
#include "Math/MathUtil.h"
#include "Math/VertexData.h"
#include <cstdint>
#include <vector>

class Cube {

public:
  const std::vector<VertexData> &GetVertices() const { return vertices_; }
  uint32_t GetVertexCount() const {
    return static_cast<uint32_t>(vertices_.size());
  }

  void Build();

private:
  std::vector<VertexData> vertices_;
};