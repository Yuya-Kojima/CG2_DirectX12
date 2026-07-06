#pragma once
#include "Framework/BaseActor.h"
#include "Actor/Enemy.h"
#include <memory>
#include <functional>
#include "Math/Vector4.h"
#include "Math/Vector3.h"

#include "Render/Object3d/Object3d.h"
#include "Render/Sprite/Sprite.h"
class SphereCollider;
class ICamera;
class Player;
class SpriteRenderer;

enum class BossPhase {
  Phase1,
  Phase2,
  Dying,
  Defeated
};

class Boss : public Enemy {
public:
  Boss();
  ~Boss() override;

  void Initialize() override;
  void InitializeUI(SpriteRenderer* spriteRenderer);
  void Update() override;
  void Draw3D() override;
  void Draw2D() override;
  void OnCollision(class Collider *other) override;

  // 表示用の3Dモデルを外から渡してセットする
  void SetModel(std::unique_ptr<Object3d> model) { model_ = std::move(model); }
  Object3d* GetModel() const { return model_.get(); }
  void SetBaseColor(const Vector4& color) { baseColor_ = color; }
  const Vector4& GetBaseColor() const { return baseColor_; }

  void SetCamera(const ICamera* camera) { camera_ = camera; }
  void SetPlayer(const Player* player) { player_ = player; }
  
  Transform& GetTransform() { return transform_; }

  // ステータスのゲッター/セッター
  int GetHP() const { return hp_; }
  void SetHP(int hp) { hp_ = hp; }
  int GetMaxHP() const { return maxHp_; }
  void SetMaxHP(int hp) { maxHp_ = hp; }
  BossPhase GetPhase() const { return phase_; }

  // ダメージを受ける処理
  void TakeDamage(int damage, bool isSelfDestruct = false) override;

private:
  void ChangePhase(BossPhase nextPhase);

  int maxHp_ = 100;
  BossPhase phase_ = BossPhase::Phase1;
  float actionTimer_ = 0.0f;
  float shotTimer_ = 0.0f;
  
  float chargeOffsetZ_ = 0.0f;
  bool isCharging_ = false;
  float chargeDuration_ = 0.0f;
  float chargeTime_ = 0.0f;
  float chargeStartOffsetZ_ = 0.0f;
  float chargeTargetOffsetZ_ = 0.0f;

  // --- UI ---
  std::unique_ptr<Sprite> hpBarBg_;
  std::unique_ptr<Sprite> hpBarFg_;
  bool isUIInitialized_ = false;

  // --- Dying演出 ---
  float dyingTimer_ = 0.0f;        // 消滅演出の経過時間
  float dyingDuration_ = 3.0f;    // 演出の総時間(秒)
  float nextExplosionTime_ = 0.0f; // 次に爆発エフェクトを出す時間
  std::function<void(const Vector3&)> onExplosionCallback_; // 爆発エフェクトのコールバック
public:
  void SetOnExplosionCallback(std::function<void(const Vector3&)> cb) { onExplosionCallback_ = cb; }
};
