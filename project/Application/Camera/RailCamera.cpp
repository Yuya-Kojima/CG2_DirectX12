#include "RailCamera.h"
#include "Math/MathUtil.h"
#include <algorithm>
#include <cmath>

RailCamera::RailCamera() {
  transform_.scale = {1.0f, 1.0f, 1.0f};
  transform_.rotate = {0.0f, 0.0f, 0.0f};
  transform_.translate = {0.0f, 0.0f, 0.0f};

  fov_ = 45.0f * (3.14159265f / 180.0f);
  aspectRatio_ = 1280.0f / 720.0f;
  nearClip_ = 0.1f;
  farClip_ = 1000.0f;

  worldMatrix_ = MakeIdentity4x4();
  viewMatrix_ = MakeIdentity4x4();
  projectionMatrix_ = MakeIdentity4x4();
  viewProjectionMatrix_ = MakeIdentity4x4();
}

void RailCamera::Initialize(const std::vector<Vector3>& waypoints) {
  waypoints_ = waypoints;
  t_ = 0.0f;
  isFinished_ = false;

  if (waypoints_.size() > 0) {
    transform_.translate = waypoints_[0];
  }
}

void RailCamera::Update() {
  if (waypoints_.size() < 2) return;

  if (!isFinished_ && isAutoMove_) {
    // 時間で進行度を進める (speed_ は 1秒間に何セグメント進むか)
    t_ += speed_ * (1.0f / 60.0f);

    float maxT = static_cast<float>(waypoints_.size() - 1);
    if (t_ >= maxT) {
      t_ = maxT;
      isFinished_ = true;
    }
  }

  // 現在の座標を計算
  Vector3 currentPos = CalcPosition(t_);
  
  // 少し先の座標を計算して進行方向(前)を求める
  Vector3 nextPos = CalcPosition(std::min(t_ + 0.01f, static_cast<float>(waypoints_.size() - 1)));
  Vector3 forward = {nextPos.x - currentPos.x, nextPos.y - currentPos.y, nextPos.z - currentPos.z};
  forward = SafeNormalize(forward);

  // 進行方向からY軸の回転(ヨー)とX軸の回転(ピッチ)を求める
  transform_.translate = currentPos;
  // atan2(x, z) でY軸回転を求める (Z+が前方の環境を想定)
  transform_.rotate.y = std::atan2(forward.x, forward.z);
  // X軸回転 (ピッチ) を求める
  float xzLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
  transform_.rotate.x = std::atan2(-forward.y, xzLen);

  // カメラ行列の更新
  worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
  viewMatrix_ = Inverse(worldMatrix_);
  projectionMatrix_ = MakePerspectiveFovMatrix(fov_, aspectRatio_, nearClip_, farClip_);
  viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}

Vector3 RailCamera::CalcPosition(float t) const {
  if (waypoints_.empty()) return {0.0f, 0.0f, 0.0f};
  if (waypoints_.size() == 1) return waypoints_[0];

  int maxIdx = static_cast<int>(waypoints_.size() - 1);
  int i = static_cast<int>(t);
  if (i >= maxIdx) {
    return waypoints_.back();
  }

  float localT = t - static_cast<float>(i);

  // 4つの制御点を取得（範囲外にならないようにClampする）
  int idx0 = std::max(i - 1, 0);
  int idx1 = i;
  int idx2 = std::min(i + 1, maxIdx);
  int idx3 = std::min(i + 2, maxIdx);

  const Vector3& p0 = waypoints_[idx0];
  const Vector3& p1 = waypoints_[idx1];
  const Vector3& p2 = waypoints_[idx2];
  const Vector3& p3 = waypoints_[idx3];

  return CatmullRom(p0, p1, p2, p3, localT);
}
