#pragma once
#include "InputKeyState.h"
#include "InputMouseState.h"
#include "Matrix4x4.h"
#include "MatrixUtility.h"
#include "Vector3.h"
#include <Windows.h>
#include <cassert>

class DebugCamera {

public:
  /// <summary>
  /// 更新処理
  /// </summary>
  void Update(const InputKeyState &input);

  Matrix4x4 GetViewMatrix() { return viewMatrix_; }

private:
  // X,Y,X軸回りのローカル回転角
  // Vector3 rotation_ = {0.0f, 0.0f, 0.0f};

  // 累積回転行列
  Matrix4x4 matRot_ = MakeIdentity4x4();

  // ローカル座標
  Vector3 translation_ = {0.0f, 0.0f, -50.0f};

  // ビュー行列
  Matrix4x4 viewMatrix_ = MakeIdentity4x4();

  // 射影行列
  Matrix4x4 projectionMatrix_ = MakeIdentity4x4();

  // マウス用
  int prevMouseX_ = 0;
  int prevMouseY_ = 0;
};
