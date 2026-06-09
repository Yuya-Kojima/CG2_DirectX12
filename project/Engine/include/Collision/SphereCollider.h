#pragma once
#include "Collider.h"
#include "Framework/BaseActor.h"
#include "Math/Geometry.h"
#include <algorithm>
#include <cmath>
#include "Render/Renderer/LineRenderer.h"

class SphereCollider : public Collider {
public:
  SphereCollider(BaseActor *owner) : Collider(owner) {}

  ShapeType GetShapeType() const override { return ShapeType::Sphere; }

  void Update() override {
    // オーナーの座標を追従させる
    worldSphere_.center = GetOwner()->GetTransform().translate;

    // オブジェクトのスケールを加味する
    Vector3 scale = GetOwner()->GetTransform().scale;
    float maxScale = (std::max)(scale.x, (std::max)(scale.y, scale.z));
    worldSphere_.radius = radius_ * maxScale;
  }

  void DrawDebug() override {
    LineRenderer* lineRenderer = LineRenderer::GetInstance();
    int segments = 16;
    float angleStep = 2.0f * 3.14159265f / segments;
    Vector4 color = {0.0f, 1.0f, 0.0f, 1.0f}; // 緑色

    for (int i = 0; i < segments; ++i) {
      float angle1 = i * angleStep;
      float angle2 = (i + 1) * angleStep;

      // XY plane
      Vector3 p1_xy = {worldSphere_.center.x + std::cos(angle1) * worldSphere_.radius,
                       worldSphere_.center.y + std::sin(angle1) * worldSphere_.radius,
                       worldSphere_.center.z};
      Vector3 p2_xy = {worldSphere_.center.x + std::cos(angle2) * worldSphere_.radius,
                       worldSphere_.center.y + std::sin(angle2) * worldSphere_.radius,
                       worldSphere_.center.z};
      lineRenderer->DrawLine(p1_xy, p2_xy, color);

      // XZ plane
      Vector3 p1_xz = {worldSphere_.center.x + std::cos(angle1) * worldSphere_.radius,
                       worldSphere_.center.y,
                       worldSphere_.center.z + std::sin(angle1) * worldSphere_.radius};
      Vector3 p2_xz = {worldSphere_.center.x + std::cos(angle2) * worldSphere_.radius,
                       worldSphere_.center.y,
                       worldSphere_.center.z + std::sin(angle2) * worldSphere_.radius};
      lineRenderer->DrawLine(p1_xz, p2_xz, color);

      // YZ plane
      Vector3 p1_yz = {worldSphere_.center.x,
                       worldSphere_.center.y + std::cos(angle1) * worldSphere_.radius,
                       worldSphere_.center.z + std::sin(angle1) * worldSphere_.radius};
      Vector3 p2_yz = {worldSphere_.center.x,
                       worldSphere_.center.y + std::cos(angle2) * worldSphere_.radius,
                       worldSphere_.center.z + std::sin(angle2) * worldSphere_.radius};
      lineRenderer->DrawLine(p1_yz, p2_yz, color);
    }
  }

  void OnCollision(Collider *other) override {
    // オーナー側のOnCollisionを呼び出して、ゲームロジックに伝達する
    GetOwner()->OnCollision(other);
  }

  // 球の取得と設定
  Sphere GetWorldSphere() const { return worldSphere_; }

  void SetRadius(float radius) { radius_ = radius; }
  float GetRadius() const { return radius_; }

private:
  Sphere worldSphere_;
  float radius_ = 1.0f; // デフォルトの半径
};
