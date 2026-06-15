#pragma once
#include "Framework/BaseActor.h"
#include "Math/Vector2.h"
#include <memory>
#include <vector>
#include <cassert>

class LockOn;
class SpriteRenderer;
class ICamera;
class Object3d;
class Sprite;
class Input;
class Enemy;

class Player : public BaseActor {
public:
  Player(const ICamera* camera);
  ~Player() override;

  void Initialize() override;
  void Update() override;
  void UpdateTransform() override;
  void ForceSnapToCamera();
  void TakeDamage(int damage);
  void Draw3D() override;
  void Draw2D() override;
  void OnCollision(class Collider *other) override;

  bool IsLockOnMode() const;

  // シーンから情報を渡すためのセッター
  void SetSpriteRenderer(SpriteRenderer *renderer) {
    spriteRenderer_ = renderer;
  }
  void SetObject3dRenderer(class Object3dRenderer* renderer) {
    object3dRenderer_ = renderer;
  }
  void SetCamera(const ICamera *camera) { 
    assert(camera);
    camera_ = camera; 
  }
  void SetInput(class Input *input) { input_ = input; }
  void SetModel(std::unique_ptr<Object3d> model) { object3d_ = std::move(model); }

  // 今回はテストとして直接ターゲットリストを渡す
  void SetEnemies(const std::vector<Enemy*>& enemies) { enemies_ = enemies; }

  int GetHp() const { return hp_; }
  bool IsDead() const { return hp_ <= 0; }

private:
  int hp_ = 3; // プロトタイプ版の仮HP（パンツァードラグーンのようなライフ制を見据える）
  std::unique_ptr<class SphereCollider> collider_;
  std::unique_ptr<LockOn> lockOn_;

  SpriteRenderer *spriteRenderer_ = nullptr;
  class Object3dRenderer *object3dRenderer_ = nullptr;
  const ICamera *camera_ = nullptr;
  class Input *input_ = nullptr;
  
  std::vector<Enemy*> enemies_;

  void FireHomingShot();
  void FireNormalShot(); // 追加

  // 照準用
  Vector2 reticlePosition_;
  
  // 多重レティクル用スプライト群
  std::vector<std::unique_ptr<Sprite>> reticleOuterSprites_; // 外側の枠（線4本）
  std::vector<std::unique_ptr<Sprite>> reticleInnerSprites_; // 内側の枠（線4本）
  float reticleOuterRot_ = 0.0f; // 外枠の回転角
  float reticleInnerRot_ = 0.0f; // 内枠の回転角
  
  // 自機の3Dモデル
  std::unique_ptr<Object3d> object3d_;

  // 攻撃用ステート
  enum class AttackState {
    Idle,      // 待機
    Pressing,  // 押下中（短押しか長押しか見極め中）
    LockOn     // ロックオンモード（長押し確定）
  };
  AttackState attackState_ = AttackState::Idle;
  float pressTimer_ = 0.0f;
};
