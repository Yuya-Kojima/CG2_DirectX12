#pragma once
#include "Framework/BaseActor.h"
#include "Actor/Enemy.h"
#include <memory>
#include <functional>
#include "Math/Vector4.h"
#include "Math/Vector3.h"

#include "Render/Object3d/Object3d.h"
#include "Render/Sprite/Sprite.h"
#include "Render/Texture/TextureManager.h"
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

  // 表示用の3Dモデルを外から渡してセットする（overrideで確実にこちらが呼ばれる）
  void SetModel(std::unique_ptr<Object3d> model) override {
    model_ = std::move(model);
    // モデルが渡された後にディゾルブ用の設定を行う
    if (model_) {
      TextureManager::GetInstance()->LoadTexture("resources/noise0.png");
      model_->SetMaskTexturePath("resources/noise0.png");
      model_->SetEnableDissolve(false);
      model_->SetDissolveThreshold(0.0f);
      model_->SetDissolveEdgeRange(0.1f); // DebugSceneのSuzanneと同じ値
      model_->SetDissolveEdgeColor({0.0f, 5.0f, 5.0f, 1.0f}); // 強いネオンシアン
    }
  }
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
  void UpdatePhase1();
  void UpdatePhase2();
  void UpdateDying();

  int maxHp_ = 100;
  BossPhase phase_ = BossPhase::Phase1;

  // ステートマシン管理
  enum class BossState { Enter, Hover, Telegraph, Attack, Cooldown, DashTelegraph, Dash, DashCooldown };
  BossState currentState_ = BossState::Enter;
  float stateTimer_ = 0.0f;
  int attackStep_ = 0;
  Vector3 startPos_ = {0.0f, 0.0f, 0.0f};
  Vector3 targetPos_ = {0.0f, 0.0f, 0.0f};

  // --- UI ---
  std::unique_ptr<Sprite> hpBarBg_;
  std::unique_ptr<Sprite> hpBarFg_;
  bool isUIInitialized_ = false;

  // --- ディゾルブ制御 ---
  bool dissolveEnabled_ = false;

  float dyingTimer_ = 0.0f;        // 消滅演出の経過時間
  float dyingDuration_ = 3.0f;    // 演出の総時間(秒)
  std::function<void(const Vector3&)> onDyingUpdateCallback_; // Dyingフェーズの毎フレームコールバック
public:
  void SetOnDyingUpdateCallback(std::function<void(const Vector3&)> cb) { onDyingUpdateCallback_ = cb; }
};
