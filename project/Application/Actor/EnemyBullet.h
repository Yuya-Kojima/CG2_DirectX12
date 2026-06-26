#pragma once
#include "Framework/BaseActor.h"
#include "Math/Vector3.h"
#include <memory>

class Player;
class Object3d;
class Object3dRenderer;

class EnemyBullet : public BaseActor {
public:
  EnemyBullet();
  ~EnemyBullet() override;

  void Initialize(Object3dRenderer* renderer, const Vector3& startPos, const Vector3& velocity, Player* player);
  
  void Update() override;
  void Draw3D() override;

private:
  std::unique_ptr<Object3d> object3d_;
  Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
  int lifeTimer_ = 0;
  Player* player_ = nullptr;
};
