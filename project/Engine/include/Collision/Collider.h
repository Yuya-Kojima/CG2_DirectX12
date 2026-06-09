#pragma once
#include "CollisionConfig.h"
#include <stdint.h>

class BaseActor;

// 全てのコライダーの親クラス
class Collider {
public:
  enum class ShapeType { Sphere, AABB, OBB };

  Collider(BaseActor *owner) : owner_(owner) {}
  virtual ~Collider() = default;

  // 当たり判定のための情報取得
  virtual ShapeType GetShapeType() const = 0;

  // オーナーの座標に追従させるための更新処理
  virtual void Update() = 0;

  // デバッグ描画用
  virtual void DrawDebug() {}

  // 衝突時のコールバック
  virtual void OnCollision(Collider *other) = 0;

  // 属性とマスクのゲッター・セッター
  void SetAttribute(uint32_t attribute) { collisionAttribute_ = attribute; }
  uint32_t GetAttribute() const { return collisionAttribute_; }

  void SetMask(uint32_t mask) { collisionMask_ = mask; }
  uint32_t GetMask() const { return collisionMask_; }

  BaseActor *GetOwner() const { return owner_; }

private:
  BaseActor *owner_ = nullptr;
  uint32_t collisionAttribute_ = kCollisionAttributeAll; // 自分の属性
  uint32_t collisionMask_ = kCollisionAttributeAll;      // 当たる相手の属性
};
