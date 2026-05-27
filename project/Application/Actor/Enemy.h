#pragma once
#include "Framework/BaseActor.h"
#include <memory>

class Object3d;
class SphereCollider;

class Enemy : public BaseActor {
public:
  Enemy();
  ~Enemy() override;

  void Initialize() override;
  void Update() override;
  void Draw3D() override;
  void OnCollision(class Collider *other) override;

  // 表示用の3Dモデルを外から渡してセットする
  void SetModel(std::unique_ptr<Object3d> model) { model_ = std::move(model); }
  Object3d* GetModel() const { return model_.get(); }

private:
  std::unique_ptr<Object3d> model_;
  std::unique_ptr<SphereCollider> collider_;
};
