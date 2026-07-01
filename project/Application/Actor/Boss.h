#pragma once
#include "Framework/BaseActor.h"
#include <memory>
#include <functional>
#include "Math/Vector4.h"
#include "Math/Vector3.h"

#include "Render/Object3d/Object3d.h"
class SphereCollider;
class ICamera;
class Player;

enum class BossPhase {
  Phase1,
  Phase2,
  Defeated
};

class Boss : public BaseActor {
public:
  Boss();
  ~Boss() override;

  void Initialize() override;
  void Update() override;
  void UpdateTransform() override;
  void Draw3D() override;
  void OnCollision(class Collider *other) override;

  // 表示用の3Dモデルを外から渡してセットする
  void SetModel(std::unique_ptr<Object3d> model) { model_ = std::move(model); }
  Object3d* GetModel() const { return model_.get(); }
  void SetBaseColor(const Vector4& color) { baseColor_ = color; }
  const Vector4& GetBaseColor() const { return baseColor_; }

  void SetCamera(const ICamera* camera) { camera_ = camera; }
  const ICamera* GetCamera() const { return camera_; }
  void SetPlayer(const Player* player) { player_ = player; }
  const Player* GetPlayer() const { return player_; }
  
  Transform& GetTransform() { return transform_; }

  // ステータスのゲッター/セッター
  int GetHP() const { return hp_; }
  void SetHP(int hp) { hp_ = hp; }
  int GetMaxHP() const { return maxHp_; }
  void SetMaxHP(int hp) { maxHp_ = hp; }
  BossPhase GetPhase() const { return phase_; }

  // ダメージを受ける処理
  void TakeDamage(int damage);

  // 死亡時のコールバック設定
  void SetOnDestroyedCallback(std::function<void()> cb) { onDestroyedCallback_ = cb; }

private:
  void ChangePhase(BossPhase nextPhase);

  std::unique_ptr<Object3d> model_;
  std::unique_ptr<SphereCollider> collider_;

  int hp_ = 100;
  int maxHp_ = 100;
  BossPhase phase_ = BossPhase::Phase1;

  const ICamera* camera_ = nullptr;
  const Player* player_ = nullptr;

  // --- 演出用パラメータ ---
  int hitFlashTimer_ = 0;  // 被弾時の点滅タイマー
  Vector4 baseColor_ = {1.0f, 1.0f, 1.0f, 1.0f}; // 基本色

  std::function<void()> onDestroyedCallback_ = nullptr;
};
