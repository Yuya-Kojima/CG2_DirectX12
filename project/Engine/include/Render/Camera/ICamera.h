#pragma once
#include "Math/MathUtil.h"

class ICamera {
public:
  virtual ~ICamera() = default;

  virtual const Matrix4x4 &GetViewMatrix() const = 0;
  virtual const Matrix4x4 &GetProjectionMatrix() const = 0;
  virtual const Matrix4x4 &GetViewProjectionMatrix() const = 0;

  virtual Vector3 GetTranslate() const = 0;
};
