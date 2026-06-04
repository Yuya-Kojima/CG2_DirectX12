#pragma once
#include "Framework/BaseActor.h"
#include <memory>
#include "Math/Vector4.h"
#include "Math/Vector3.h"

class Object3d;
class SphereCollider;

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

  // 移動方向のセッター
  void SetMoveDirection(const Vector3& dir) { moveDirection_ = dir; }

  // ダメージを受ける処理
  void TakeDamage(int damage);

private:
  std::unique_ptr<Object3d> model_;
  std::unique_ptr<SphereCollider> collider_;

  // --- 敵のステータス ---
  int hp_ = 3;             // 体力
  float speed_ = 0.5f;     // 移動速度
  Vector3 moveDirection_ = {0.0f, 0.0f, -1.0f}; // 進行方向ベクトル（デフォルトはワールドZマイナス方向）
  
  // --- 演出用パラメータ ---
  int hitFlashTimer_ = 0;  // 被弾時の点滅タイマー
  Vector4 baseColor_ = {1.0f, 1.0f, 1.0f, 1.0f}; // 基本色
};
