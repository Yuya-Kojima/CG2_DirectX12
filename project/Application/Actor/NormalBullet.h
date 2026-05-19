#pragma once
#include "Framework/BaseActor.h"
#include "Math/Vector3.h"
#include <memory>
#include <vector>

class Enemy;
class Object3d;
class Object3dRenderer;

class NormalBullet : public BaseActor {
public:
  NormalBullet();
  ~NormalBullet() override;

  /// <summary>
  /// 通常弾の初期化
  /// </summary>
  void Initialize(Object3dRenderer* renderer, const Vector3& startPos, const Vector3& velocity, const std::vector<Enemy*>& enemies);
  
  void Update() override;
  void Draw3D() override;

  bool IsDead() const { return isDead_; }

private:
  std::unique_ptr<Object3d> object3d_;
  std::vector<Enemy*> enemies_; // 当たり判定用
  
  Vector3 velocity_;
  int lifeTimer_ = 60; // 寿命（約1秒で消滅）

  bool isDead_ = false;
};
