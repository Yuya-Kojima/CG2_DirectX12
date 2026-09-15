#include "Render/Mesh/MeshGenerator.h"
#include "Math/MathUtil.h"
#include <cmath>
#include <numbers>

namespace RC {

Model::ModelData MeshGenerator::GeneratePlane(float width, float height,
                                              uint32_t segmentsW, uint32_t segmentsH) {
  Model::ModelData data;
  float halfW = width * 0.5f;
  float halfH = height * 0.5f;

  for (uint32_t y = 0; y <= segmentsH; ++y) {
    for (uint32_t x = 0; x <= segmentsW; ++x) {
      float u = (float)x / segmentsW;
      float v = (float)y / segmentsH;
      Model::VertexData vtx;
      vtx.position = {u * width - halfW, 0.0f, (1.0f - v) * height - halfH, 1.0f};
      vtx.texcoord = {u, v};
      vtx.normal = {0.0f, 1.0f, 0.0f};
      data.vertices.push_back(vtx);
    }
  }

  for (uint32_t y = 0; y < segmentsH; ++y) {
    for (uint32_t x = 0; x < segmentsW; ++x) {
      uint32_t lb = y * (segmentsW + 1) + x;
      uint32_t rb = lb + 1;
      uint32_t lt = (y + 1) * (segmentsW + 1) + x;
      uint32_t rt = lt + 1;

      data.indices.push_back(lb);
      data.indices.push_back(rb);
      data.indices.push_back(lt);

      data.indices.push_back(rb);
      data.indices.push_back(rt);
      data.indices.push_back(lt);
    }
  }

  data.rootNode.name = "Plane";
  data.rootNode.localMatrix = MakeIdentity4x4();
  return data;
}

Model::ModelData MeshGenerator::GenerateBox(float width, float height, float depth) {
  Model::ModelData data;
  float w2 = width * 0.5f;
  float h2 = height * 0.5f;
  float d2 = depth * 0.5f;

  struct Face {
    Vector3 normal;
    Vector3 tangent;
    Vector3 bitangent;
  };
  Face faces[6] = {
      {{0, 0, 1},  {-1, 0, 0}, {0, 1, 0}}, // Front
      {{0, 0, -1}, {1, 0, 0},  {0, 1, 0}}, // Back
      {{0, 1, 0},  {1, 0, 0},  {0, 0, 1}}, // Top
      {{0, -1, 0}, {1, 0, 0},  {0, 0, -1}},// Bottom 
      {{1, 0, 0},  {0, 0, 1},  {0, 1, 0}}, // Right
      {{-1, 0, 0}, {0, 0, -1}, {0, 1, 0}}, // Left
  };

  for (int i = 0; i < 6; ++i) {
    uint32_t baseIdx = (uint32_t)data.vertices.size();
    for (int y = 0; y <= 1; ++y) {
      for (int x = 0; x <= 1; ++x) {
        float fx = x * 2.0f - 1.0f;
        float fy = y * 2.0f - 1.0f;
        Model::VertexData vtx;
        Vector3 pos = faces[i].normal + faces[i].tangent * fx + faces[i].bitangent * fy;
        vtx.position = {pos.x * w2, pos.y * h2, pos.z * d2, 1.0f};
        vtx.normal = faces[i].normal;
        vtx.texcoord = {(float)x, 1.0f - y};
        data.vertices.push_back(vtx);
      }
    }
    // CW winding
    data.indices.push_back(baseIdx + 0);
    data.indices.push_back(baseIdx + 2);
    data.indices.push_back(baseIdx + 3);
    data.indices.push_back(baseIdx + 0);
    data.indices.push_back(baseIdx + 3);
    data.indices.push_back(baseIdx + 1);
  }

  data.rootNode.name = "Box";
  data.rootNode.localMatrix = MakeIdentity4x4();
  return data;
}

Model::ModelData MeshGenerator::GenerateCircle(float radius, uint32_t segments) {
  Model::ModelData data;
  data.vertices.push_back({.position = {0, 0, 0, 1}, .texcoord = {0.5f, 0.5f}, .normal = {0, 1, 0}});

  const float pi2 = 2.0f * std::numbers::pi_v<float>;
  for (uint32_t i = 0; i <= segments; ++i) {
    float rad = (float)i / segments * pi2;
    float c = std::cos(rad);
    float s = std::sin(rad);
    Model::VertexData vtx;
    vtx.position = {c * radius, 0.0f, s * radius, 1.0f};
    vtx.normal = {0, 1, 0};
    vtx.texcoord = {c * 0.5f + 0.5f, s * 0.5f + 0.5f};
    data.vertices.push_back(vtx);
  }

  for (uint32_t i = 0; i < segments; ++i) {
    data.indices.push_back(0);
    data.indices.push_back(i + 2);
    data.indices.push_back(i + 1);
  }

  data.rootNode.name = "Circle";
  data.rootNode.localMatrix = MakeIdentity4x4();
  return data;
}

Model::ModelData MeshGenerator::GenerateRing(float innerRadius, float outerRadius, uint32_t segments) {
  Model::ModelData data;
  const float pi2 = 2.0f * std::numbers::pi_v<float>;
  for (uint32_t i = 0; i <= segments; ++i) {
    float rad = (float)i / segments * pi2;
    float c = std::cos(rad);
    float s = std::sin(rad);
    float u = (float)i / segments;

    Model::VertexData vInner;
    vInner.position = {c * innerRadius, 0.0f, s * innerRadius, 1.0f};
    vInner.normal = {0, 1, 0};
    vInner.texcoord = {u, 1.0f};
    data.vertices.push_back(vInner);

    Model::VertexData vOuter;
    vOuter.position = {c * outerRadius, 0.0f, s * outerRadius, 1.0f};
    vOuter.normal = {0, 1, 0};
    vOuter.texcoord = {u, 0.0f};
    data.vertices.push_back(vOuter);
  }

  for (uint32_t i = 0; i < segments; ++i) {
    uint32_t base = i * 2;
    data.indices.push_back(base + 0);
    data.indices.push_back(base + 2);
    data.indices.push_back(base + 3);
    data.indices.push_back(base + 0);
    data.indices.push_back(base + 3);
    data.indices.push_back(base + 1);
  }

  data.rootNode.name = "Ring";
  data.rootNode.localMatrix = MakeIdentity4x4();
  return data;
}

Model::ModelData MeshGenerator::GenerateSphere(float radius, uint32_t slices, uint32_t stacks) {
  Model::ModelData data;
  const float pi = std::numbers::pi_v<float>;
  const float pi2 = 2.0f * pi;

  for (uint32_t y = 0; y <= stacks; ++y) {
    float phi = (float)y / stacks * pi;
    for (uint32_t x = 0; x <= slices; ++x) {
      float theta = (float)x / slices * pi2;
      Model::VertexData vtx;
      float sinPhi = std::sin(phi);
      float cosPhi = std::cos(phi);
      float sinTheta = std::sin(theta);
      float cosTheta = std::cos(theta);

      vtx.normal = {sinPhi * cosTheta, cosPhi, sinPhi * sinTheta};
      vtx.position = {vtx.normal.x * radius, vtx.normal.y * radius, vtx.normal.z * radius, 1.0f};
      vtx.texcoord = {(float)x / slices, (float)y / stacks};
      data.vertices.push_back(vtx);
    }
  }

  for (uint32_t y = 0; y < stacks; ++y) {
    for (uint32_t x = 0; x < slices; ++x) {
      uint32_t lb = y * (slices + 1) + x;
      uint32_t rb = lb + 1;
      uint32_t lt = (y + 1) * (slices + 1) + x;
      uint32_t rt = lt + 1;

      data.indices.push_back(lb);
      data.indices.push_back(rb);
      data.indices.push_back(lt);
      data.indices.push_back(lt);
      data.indices.push_back(rb);
      data.indices.push_back(rt);
    }
  }
  data.rootNode.name = "Sphere";
  data.rootNode.localMatrix = MakeIdentity4x4();
  return data;
}

Model::ModelData MeshGenerator::GenerateCylinder(float radius, float height, uint32_t segments) {
  Model::ModelData data;
  float h2 = height * 0.5f;
  const float pi2 = 2.0f * std::numbers::pi_v<float>;

  // 側面 (+Z方向に伸びる円柱)
  for (uint32_t i = 0; i <= segments; ++i) {
    float rad = (float)i / segments * pi2;
    float c = std::cos(rad);
    float s = std::sin(rad);
    float u = (float)i / segments;

    // 後端 (-Z)
    Model::VertexData v0;
    v0.position = {c * radius, s * radius, -h2, 1.0f};
    v0.normal = {c, s, 0.0f};
    v0.texcoord = {u, 1.0f};
    data.vertices.push_back(v0);

    // 先端 (+Z)
    Model::VertexData v1;
    v1.position = {c * radius, s * radius, h2, 1.0f};
    v1.normal = {c, s, 0.0f};
    v1.texcoord = {u, 0.0f};
    data.vertices.push_back(v1);
  }

  for (uint32_t i = 0; i < segments; ++i) {
    uint32_t base = i * 2;
    data.indices.push_back(base + 0);
    data.indices.push_back(base + 3);
    data.indices.push_back(base + 1);
    data.indices.push_back(base + 0);
    data.indices.push_back(base + 2);
    data.indices.push_back(base + 3);
  }

  // 先端キャップ (+Z)
  uint32_t topCenterIdx = (uint32_t)data.vertices.size();
  data.vertices.push_back({.position = {0, 0, h2, 1}, .texcoord = {0.5f, 0.5f}, .normal = {0, 0, 1}});
  uint32_t topRingStart = (uint32_t)data.vertices.size();
  for (uint32_t i = 0; i <= segments; ++i) {
    float rad = (float)i / segments * pi2;
    float c = std::cos(rad);
    float s = std::sin(rad);
    data.vertices.push_back({.position = {c * radius, s * radius, h2, 1.0f}, .texcoord = {c * 0.5f + 0.5f, s * 0.5f + 0.5f}, .normal = {0, 0, 1}});
  }

  // 後端キャップ (-Z)
  uint32_t bottomCenterIdx = (uint32_t)data.vertices.size();
  data.vertices.push_back({.position = {0, 0, -h2, 1}, .texcoord = {0.5f, 0.5f}, .normal = {0, 0, -1}});
  uint32_t bottomRingStart = (uint32_t)data.vertices.size();
  for (uint32_t i = 0; i <= segments; ++i) {
    float rad = (float)i / segments * pi2;
    float c = std::cos(rad);
    float s = std::sin(rad);
    data.vertices.push_back({.position = {c * radius, s * radius, -h2, 1.0f}, .texcoord = {c * 0.5f + 0.5f, s * 0.5f + 0.5f}, .normal = {0, 0, -1}});
  }

  for (uint32_t i = 0; i < segments; ++i) {
    // 先端 (+Z向き、時計回り)
    data.indices.push_back(topCenterIdx);
    data.indices.push_back(topRingStart + i);
    data.indices.push_back(topRingStart + i + 1);

    // 後端 (-Z向き、時計回り)
    data.indices.push_back(bottomCenterIdx);
    data.indices.push_back(bottomRingStart + i + 1);
    data.indices.push_back(bottomRingStart + i);
  }

  data.rootNode.name = "Cylinder";
  data.rootNode.localMatrix = MakeIdentity4x4();
  return data;
}

Model::ModelData MeshGenerator::GenerateCapsule(float radius, float height, uint32_t slices, uint32_t stacks) {
  Model::ModelData data;
  float cylinderH = (std::max)(0.0f, height - 2.0f * radius);
  float h2 = cylinderH * 0.5f;

  const float pi = std::numbers::pi_v<float>;
  const float pi2 = 2.0f * pi;

  for (uint32_t y = 0; y <= stacks; ++y) {
    float phi = (float)y / stacks * pi;
    float sinPhi = std::sin(phi);
    float cosPhi = std::cos(phi);
    float yPos = cosPhi * radius;

    // 半球によるオフセット
    if (y <= stacks / 2) yPos += h2;
    else yPos -= h2;

    for (uint32_t x = 0; x <= slices; ++x) {
      float theta = (float)x / slices * pi2;
      float sinTheta = std::sin(theta);
      float cosTheta = std::cos(theta);

      Model::VertexData vtx;
      vtx.normal = {sinPhi * cosTheta, cosPhi, sinPhi * sinTheta};
      vtx.position = {vtx.normal.x * radius, yPos, vtx.normal.z * radius, 1.0f};
      vtx.texcoord = {(float)x / slices, (float)y / stacks};
      data.vertices.push_back(vtx);
    }
  }

  for (uint32_t y = 0; y < stacks; ++y) {
    for (uint32_t x = 0; x < slices; ++x) {
      uint32_t lb = y * (slices + 1) + x;
      uint32_t rb = lb + 1;
      uint32_t lt = (y + 1) * (slices + 1) + x;
      uint32_t rt = lt + 1;

      data.indices.push_back(lb);
      data.indices.push_back(rb);
      data.indices.push_back(lt);
      data.indices.push_back(lt);
      data.indices.push_back(rb);
      data.indices.push_back(rt);
    }
  }
  data.rootNode.name = "Capsule";
  data.rootNode.localMatrix = MakeIdentity4x4();
  return data;
}

Model::ModelData MeshGenerator::GenerateCone(float radius, float height, uint32_t segments) {
  Model::ModelData data;
  float h2 = height * 0.5f;
  const float pi2 = 2.0f * std::numbers::pi_v<float>;

  data.vertices.push_back({.position = {0, h2, 0, 1}, .texcoord = {0.5f, 0.0f}, .normal = {0, 1, 0}}); // Top
  for (uint32_t i = 0; i <= segments; ++i) {
    float rad = (float)i / segments * pi2;
    float c = std::cos(rad);
    float s = std::sin(rad);
    Model::VertexData v;
    v.position = {c * radius, -h2, s * radius, 1.0f};
    v.normal = {c, 0.5f, s};
    float len = std::sqrt(v.normal.x * v.normal.x + v.normal.y * v.normal.y + v.normal.z * v.normal.z);
    v.normal.x /= len; v.normal.y /= len; v.normal.z /= len;
    v.texcoord = {(float)i / segments, 1.0f};
    data.vertices.push_back(v);
  }
  for (uint32_t i = 0; i < segments; ++i) {
    data.indices.push_back(0);
    data.indices.push_back(i + 2);
    data.indices.push_back(i + 1);
  }

  // Bottom cap
  uint32_t centerIdx = (uint32_t)data.vertices.size();
  data.vertices.push_back({.position = {0, -h2, 0, 1}, .texcoord = {0.5f, 0.5f}, .normal = {0, -1, 0}});
  uint32_t ringStart = (uint32_t)data.vertices.size();
  for (uint32_t i = 0; i <= segments; ++i) {
    float rad = (float)i / segments * pi2;
    float c = std::cos(rad);
    float s = std::sin(rad);
    data.vertices.push_back({.position = {c * radius, -h2, s * radius, 1.0f}, .texcoord = {c * 0.5f + 0.5f, s * 0.5f + 0.5f}, .normal = {0, -1, 0}});
  }
  for (uint32_t i = 0; i < segments; ++i) {
    data.indices.push_back(centerIdx);
    data.indices.push_back(ringStart + i);
    data.indices.push_back(ringStart + i + 1);
  }

  data.rootNode.name = "Cone";
  data.rootNode.localMatrix = MakeIdentity4x4();
  return data;
}

Model::ModelData MeshGenerator::GenerateTorus(float majorRadius, float minorRadius, uint32_t majorSegments, uint32_t minorSegments) {
  Model::ModelData data;
  const float pi2 = 2.0f * std::numbers::pi_v<float>;

  for (uint32_t j = 0; j <= majorSegments; ++j) {
    float majorRad = (float)j / majorSegments * pi2;
    Vector3 majorPos = {std::cos(majorRad), 0, std::sin(majorRad)};
    for (uint32_t i = 0; i <= minorSegments; ++i) {
      float minorRad = (float)i / minorSegments * pi2;
      float c = std::cos(minorRad);
      float s = std::sin(minorRad);
      
      Vector3 normal = majorPos * c + Vector3{0, s, 0};
      Vector3 pos = majorPos * majorRadius + normal * minorRadius;
      
      Model::VertexData vtx;
      vtx.position = {pos.x, pos.y, pos.z, 1.0f};
      vtx.normal = normal;
      vtx.texcoord = {(float)j / majorSegments, (float)i / minorSegments};
      data.vertices.push_back(vtx);
    }
  }
  for (uint32_t j = 0; j < majorSegments; ++j) {
    for (uint32_t i = 0; i < minorSegments; ++i) {
      uint32_t lb = j * (minorSegments + 1) + i;
      uint32_t rb = (j + 1) * (minorSegments + 1) + i;
      uint32_t lt = lb + 1;
      uint32_t rt = rb + 1;

      data.indices.push_back(lb);
      data.indices.push_back(lt);
      data.indices.push_back(rb);

      data.indices.push_back(lt);
      data.indices.push_back(rt);
      data.indices.push_back(rb);
    }
  }
  data.rootNode.name = "Torus";
  data.rootNode.localMatrix = MakeIdentity4x4();
  return data;
}

} // namespace RC
