#include "Render/Primitive/Cube.h"

void Cube::Build() {

  vertices_.clear();
  vertices_.reserve(36);

  const Vector2 uv = {0.0f, 0.0f};
  const Vector3 n = {0.0f, 0.0f, 0.0f};

  //==============================
  // 右面 (+X)
  // 内側向き
  //==============================
  vertices_.push_back(VertexData{{1.0f, 1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, 1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, -1.0f, 1.0f, 1.0f}, uv, n});

  vertices_.push_back(VertexData{{1.0f, -1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, 1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, -1.0f, -1.0f, 1.0f}, uv, n});

  //==============================
  // 左面 (-X)
  //==============================
  vertices_.push_back(VertexData{{-1.0f, 1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{-1.0f, 1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{-1.0f, -1.0f, -1.0f, 1.0f}, uv, n});

  vertices_.push_back(VertexData{{-1.0f, -1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{-1.0f, 1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{-1.0f, -1.0f, 1.0f, 1.0f}, uv, n});

  //==============================
  // 前面 (+Z)
  //==============================
  vertices_.push_back(VertexData{{-1.0f, 1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, 1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{-1.0f, -1.0f, 1.0f, 1.0f}, uv, n});

  vertices_.push_back(VertexData{{-1.0f, -1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, 1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, -1.0f, 1.0f, 1.0f}, uv, n});

  //==============================
  // 背面 (-Z)
  //==============================
  vertices_.push_back(VertexData{{1.0f, 1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{-1.0f, 1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, -1.0f, -1.0f, 1.0f}, uv, n});

  vertices_.push_back(VertexData{{1.0f, -1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{-1.0f, 1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{-1.0f, -1.0f, -1.0f, 1.0f}, uv, n});

  //==============================
  // 上面 (+Y)
  //==============================
  vertices_.push_back(VertexData{{-1.0f, 1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, 1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{-1.0f, 1.0f, 1.0f, 1.0f}, uv, n});

  vertices_.push_back(VertexData{{-1.0f, 1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, 1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, 1.0f, 1.0f, 1.0f}, uv, n});

  //==============================
  // 下面 (-Y)
  //==============================
  vertices_.push_back(VertexData{{-1.0f, -1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, -1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{-1.0f, -1.0f, -1.0f, 1.0f}, uv, n});

  vertices_.push_back(VertexData{{-1.0f, -1.0f, -1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, -1.0f, 1.0f, 1.0f}, uv, n});
  vertices_.push_back(VertexData{{1.0f, -1.0f, -1.0f, 1.0f}, uv, n});
}