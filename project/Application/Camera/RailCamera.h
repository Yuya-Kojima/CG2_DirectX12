#pragma once
#include "Render/Camera/ICamera.h"
#include "Math/Matrix4x4.h"
#include "Math/Transform.h"
#include "Math/Vector3.h"
#include <vector>

/// <summary>
/// スプライン曲線（Catmull-Rom）に沿って移動するレールカメラ
/// </summary>
class RailCamera : public ICamera {
public:
  RailCamera();
  ~RailCamera() override = default;

  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="waypoints">通過する座標のリスト</param>
  void Initialize(const std::vector<Vector3>& waypoints);

  /// <summary>
  /// 毎フレームの更新処理
  /// </summary>
  void Update();

  // --- ICamera インターフェースの実装 ---
  const Matrix4x4& GetViewMatrix() const override { return viewMatrix_; }
  const Matrix4x4& GetProjectionMatrix() const override { return projectionMatrix_; }
  const Matrix4x4& GetViewProjectionMatrix() const override { return viewProjectionMatrix_; }
  Vector3 GetTranslate() const override { return transform_.translate; }

  // ゲッター/セッター
  void SetSpeed(float speed) { speed_ = speed; }
  const Vector3& GetRotate() const { return transform_.rotate; }
  bool IsFinished() const { return isFinished_; }

  /// <summary>
  /// 進行度tから現在の座標を計算する
  /// </summary>
  Vector3 CalcPosition(float t) const;

private:
  Transform transform_;
  Matrix4x4 worldMatrix_;
  Matrix4x4 viewMatrix_;
  Matrix4x4 projectionMatrix_;
  Matrix4x4 viewProjectionMatrix_;

  float fov_;
  float aspectRatio_;
  float nearClip_;
  float farClip_;

  // レール移動用
  std::vector<Vector3> waypoints_; // 通過ポイント
  float t_ = 0.0f; // 現在の進行度
  float speed_ = 0.5f; // 進行スピード
  bool isFinished_ = false;

};
