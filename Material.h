#pragma once
#include "Matrix4x4.h"
#include "Vector4.h"
#include <cstdint>

struct Material {
  Vector4 color;
  int32_t lightingMode;
  float padding[3];
  Matrix4x4 uvTransform;
};

enum LightingMode {
  None,
  Lambert,
  HarfLambert,
};