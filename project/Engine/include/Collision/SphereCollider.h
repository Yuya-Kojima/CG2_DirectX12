#pragma once
#include "Collider.h"
#include "Framework/BaseActor.h"
#include "Math/Geometry.h"

class SphereCollider : public Collider {
public:
  SphereCollider(BaseActor *owner) : Collider(owner) {}

  ShapeType GetShapeType() const override { return ShapeType::Sphere; }

  void Update() override {
    // オーナーの座標を球の中心に追従させる
    worldSphere_.center = GetOwner()->GetTransform().translate;

    // オブジェクトのスケールを加味する場合はここに記述することも可能です
    worldSphere_.radius = radius_;
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
