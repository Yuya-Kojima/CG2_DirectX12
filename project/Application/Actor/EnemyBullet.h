#pragma once
#include "Framework/BaseActor.h"
#include "Math/Vector3.h"
#include <memory>

class Player;
class Object3d;
class Object3dRenderer;
class SphereCollider;

enum class EnemyBulletType {
  NormalDestructible, // 通常ショットで破壊可能
  LockOnDestructible, // ロックオン可能
  Indestructible      // 破壊不可
};

class EnemyBullet : public BaseActor {
public:
  EnemyBullet();
  ~EnemyBullet() override;

  void Initialize(Object3dRenderer* renderer, const Vector3& startPos, const Vector3& velocity, Player* player, EnemyBulletType type = EnemyBulletType::NormalDestructible);
  
  void Update() override;
  void Draw3D() override;
  void OnCollision(class Collider* other) override;

  EnemyBulletType GetBulletType() const { return type_; }
  void SetBulletType(EnemyBulletType type) { type_ = type; }
  void SetSwarmWait(int frames) { swarmWaitFrames_ = frames; }

private:
  std::unique_ptr<Object3d> object3d_;
  std::unique_ptr<SphereCollider> collider_;
  Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
  int lifeTimer_ = 0;
  int hp_ = 1;
  Player* player_ = nullptr;
  EnemyBulletType type_ = EnemyBulletType::NormalDestructible;

  // ミサイル用パラメータ
  float homingStrength_ = 0.0f;
  int aliveFrames_ = 0;
  int swarmWaitFrames_ = 0; 
};
