#pragma once
#include "Math/MathUtil.h"

class ICamera {
public:
  virtual ~ICamera() = default;

  virtual const Matrix4x4 &GetViewMatrix() const = 0;
  virtual const Matrix4x4 &GetProjectionMatrix() const = 0;
  virtual const Matrix4x4 &GetViewProjectionMatrix() const = 0;

  virtual Vector3 GetTranslate() const = 0;

  // ビュー行列からカメラの各軸ベクトルを計算して返すヘルパー関数
  Vector3 GetRight() const {
      Matrix4x4 invView = Inverse(GetViewMatrix());
      return {invView.m[0][0], invView.m[0][1], invView.m[0][2]};
  }
  Vector3 GetUp() const {
      Matrix4x4 invView = Inverse(GetViewMatrix());
      return {invView.m[1][0], invView.m[1][1], invView.m[1][2]};
  }
  Vector3 GetForward() const {
      Matrix4x4 invView = Inverse(GetViewMatrix());
      return {invView.m[2][0], invView.m[2][1], invView.m[2][2]};
  }
};
