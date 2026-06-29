#pragma once
#include "Framework/BaseActor.h"
#include "Math/Vector2.h"
#include <cassert>
#include <memory>
#include <vector>

class LockOn;
class SpriteRenderer;
class ICamera;
class Object3d;
class Sprite;
class Input;
class Enemy;

class Player : public BaseActor {
public:
  Player(const ICamera *camera);
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
  void SetObject3dRenderer(class Object3dRenderer *renderer) {
    object3dRenderer_ = renderer;
  }
  void SetCamera(const ICamera *camera) {
    assert(camera);
    camera_ = camera;
  }
  void SetInput(class Input *input) { input_ = input; }
  void SetModel(std::unique_ptr<Object3d> model) {
    object3d_ = std::move(model);
  }

  // 今回はテストとして直接ターゲットリストを渡す
  void SetEnemies(const std::vector<Enemy *> &enemies) { enemies_ = enemies; }

  int GetHp() const { return hp_; }
  bool IsDead() const { return hp_ <= 0; }

  // --- アクション・軌道調整用パラメータ ---
  struct ActionConfig {
    float lockOnRadius = 100.0f;           // ロックオンの吸いつき範囲
    float homingSpeed = 1.5f;              // ホーミング弾のスピード
    int homingFallTime = 165;              // 追尾を開始するまでのフレーム
    float homingStrengthIncrease = 0.015f; // 追尾のカーブの鋭さ
    float homingStrengthMax = 0.25f;       // 旋回力(追尾力)
    float reticleAcceleration = 2.5f;      // 照準の加速度
    float reticleFriction = 0.85f;          // 照準の摩擦力
    float reticleMaxSpeed = 25.0f;          // 照準の最高移動速度
    float rollStrength = 4.0f;             // ロールの強さ
    float pitchStrength = 2.0f;            // ピッチの追加強度
    float yawStrength = 1.5f;              // ヨーの追加強度
    float rollLerp = 0.15f;                // ロール補間速度
    float pitchLerp = 0.15f;               // ピッチ補間速度
    float yawLerp = 0.15f;                 // ヨー補間速度
  };
  ActionConfig &GetActionConfig() { return actionConfig_; }

  void SaveActionConfig();
  void LoadActionConfig();

  bool IsActionConfigDirty() const { return isActionConfigDirty_; }
  void SetActionConfigDirty(bool dirty) { isActionConfigDirty_ = dirty; }

  // 初期化時に設定ファイルをロードするか制御するフラグのセッター
  void SetLoadConfigOnInitialize(bool load) {
    loadConfigOnInitialize_ = load;
  }

private:
  ActionConfig actionConfig_;
  bool isActionConfigDirty_ = false;
  bool loadConfigOnInitialize_ = true;

  int hp_ =
      3; // プロトタイプ版の仮HP（パンツァードラグーンのようなライフ制を見据える）
  std::unique_ptr<class SphereCollider> collider_;
  std::unique_ptr<LockOn> lockOn_;

  SpriteRenderer *spriteRenderer_ = nullptr;
  class Object3dRenderer *object3dRenderer_ = nullptr;
  const ICamera *camera_ = nullptr;
  class Input *input_ = nullptr;

  std::vector<Enemy *> enemies_;

  void FireHomingShot();
  void FireNormalShot(); // 追加

  // 照準用
  Vector2 reticlePosition_;
  Vector2 reticleVelocity_ = {0.0f, 0.0f}; // 照準の移動速度（慣性用）
  
  int invincibleTimer_ = 0; // 無敵時間のタイマー

  // 多重レティクル用スプライト群
  std::vector<std::unique_ptr<Sprite>>
      reticleOuterSprites_; // 外側の枠（線4本）
  std::vector<std::unique_ptr<Sprite>>
      reticleInnerSprites_;      // 内側の枠（線4本）
  float reticleOuterRot_ = 0.0f; // 外枠の回転角
  float reticleInnerRot_ = 0.0f; // 内枠の回転角

  // 自機の3Dモデル
  std::unique_ptr<Object3d> object3d_;

  // 攻撃用ステート
  enum class AttackState {
    Idle,     // 待機
    Pressing, // 押下中（短押しか長押しか見極め中）
    LockOn    // ロックオンモード（長押し確定）
  };
  AttackState attackState_ = AttackState::Idle;
  float pressTimer_ = 0.0f;
};
