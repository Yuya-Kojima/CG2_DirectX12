#pragma once
#include "Input/InputKeyState.h"
#include "Input/InputMouseState.h"
#include "Math/MathUtil.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include "Render/Camera/GameCamera.h"
#include "Render/Camera/ICamera.h"
#include <Windows.h>
#include <cassert>

class DebugCamera {

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(const Vector3 &translate);

  /// <summary>
  /// 更新処理
  /// </summary>
  void Update(const InputKeyState &input);

  const ICamera *GetCamera() const { return &camera_; }

private:
  GameCamera camera_;

  // 累積回転行列
  Matrix4x4 matRot_ = MakeIdentity4x4();

  // ローカル座標
  Vector3 rotate_{0.3f, 0.0f, 0.0f};
  Vector3 translation_ = {0.0f, 0.0f, -50.0f};

  // ビュー行列
  Matrix4x4 viewMatrix_ = MakeIdentity4x4();

  // 射影行列
  // Matrix4x4 projectionMatrix_ = MakeIdentity4x4();

  // マウス用
  // int prevMouseX_ = 0;
  // int prevMouseY_ = 0;
};
