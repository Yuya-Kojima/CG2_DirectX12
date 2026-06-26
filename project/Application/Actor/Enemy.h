#pragma once
#include "Framework/BaseActor.h"
#include <memory>
#include <functional>
#include "Math/Vector4.h"
#include "Math/Vector3.h"

#include "Render/Object3d/Object3d.h"
class SphereCollider;
class ICamera;
class IParticleEmitter;
class ParticleEmitter;
class Player;
#include "Behavior/IEnemyBehavior.h"

enum class MoveType {
  Straight,
  Parallel,
  SineWave,
  Stationary,
  Fighter,
  Meteor,
  Strafe,
  Turret
};


class Enemy : public BaseActor {
public:
  Enemy();
  ~Enemy() override;

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

  // 移動方向・軌道のセッター/ゲッター
  void SetMoveDirection(const Vector3& dir) { moveDirection_ = dir; }
  const Vector3& GetMoveDirection() const { return moveDirection_; }
  void SetMoveType(MoveType type) { moveType_ = type; }
  MoveType GetMoveType() const { return moveType_; }
  void SetBehavior(std::unique_ptr<IEnemyBehavior> behavior) { behavior_ = std::move(behavior); }
  
  void SetCamera(const ICamera* camera) { camera_ = camera; }
  const ICamera* GetCamera() const { return camera_; }
  void SetSpawnOffset(const Vector3& offset) { spawnOffset_ = offset; }
  const Vector3& GetSpawnOffset() const { return spawnOffset_; }
  float GetAliveTime() const { return aliveTime_; }
  void SetPlayer(const Player* player) { player_ = player; }
  const Player* GetPlayer() const { return player_; }
  
  // Transformへのアクセス（BaseActorのprotectedメンバ）
  Transform& GetTransform() { return transform_; }

  // ステータスのゲッター/セッター
  int GetHP() const { return hp_; }
  void SetHP(int hp) { hp_ = hp; }
  float GetSpeed() const { return speed_; }
  void SetSpeed(float speed) { speed_ = speed; }

  // ダメージを受ける処理
  void TakeDamage(int damage);

  // 撃破時のコールバック設定
  void SetOnDestroyedCallback(std::function<void()> cb) { onDestroyedCallback_ = cb; }

private:
  std::unique_ptr<Object3d> model_;
  std::unique_ptr<SphereCollider> collider_;

  // --- 敵のステータス ---
  int hp_ = 3;             // 体力
  float speed_ = 0.5f;     // 移動速度
  Vector3 moveDirection_ = {0.0f, 0.0f, -1.0f}; // 進行方向ベクトル（デフォルトはワールドZマイナス方向）
  MoveType moveType_ = MoveType::Straight;
  std::unique_ptr<IEnemyBehavior> behavior_;
  const ICamera* camera_ = nullptr;
  const Player* player_ = nullptr;
  Vector3 spawnOffset_ = {0.0f, 0.0f, 0.0f};
  float aliveTime_ = 0.0f;
  
  // --- 演出用パラメータ ---
  int hitFlashTimer_ = 0;  // 被弾時の点滅タイマー
  Vector4 baseColor_ = {1.0f, 1.0f, 1.0f, 1.0f}; // 基本色

  std::function<void()> onDestroyedCallback_ = nullptr;
};
