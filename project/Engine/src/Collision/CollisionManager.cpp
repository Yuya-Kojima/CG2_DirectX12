#include "Collision/CollisionManager.h"
#include "Collision/Collider.h"
#include "Collision/SphereCollider.h"
#include "Math/CollisionMath.h"

CollisionManager *CollisionManager::GetInstance() {
  static CollisionManager instance;
  return &instance;
}

void CollisionManager::Initialize() { colliders_.clear(); }

void CollisionManager::Register(Collider *collider) {
  colliders_.push_back(collider);
}

void CollisionManager::Remove(Collider *collider) {
  colliders_.remove(collider);
}

void CollisionManager::Clear() { colliders_.clear(); }

void CollisionManager::Update() {
  // 全コライダーの座標を更新（オーナーのTransformに追従）
  for (Collider *collider : colliders_) {
    collider->Update();
  }

  // 総当り判定
  CheckAllCollisions();
}

void CollisionManager::CheckAllCollisions() {
  auto itA = colliders_.begin();
  for (; itA != colliders_.end(); ++itA) {
    Collider *colliderA = *itA;

    auto itB = itA;
    ++itB;
    for (; itB != colliders_.end(); ++itB) {
      Collider *colliderB = *itB;

      // 属性とマスクを使ったフィルタリング
      // Aの属性がBのマスクに含まれていない、またはBの属性がAのマスクに含まれていない場合は計算スキップ
      if (!(colliderA->GetAttribute() & colliderB->GetMask()) ||
          !(colliderB->GetAttribute() & colliderA->GetMask())) {
        continue;
      }

      // 形状ごとの判定
      bool isHit = false;

      // 球と球 の判定
      if (colliderA->GetShapeType() == Collider::ShapeType::Sphere &&
          colliderB->GetShapeType() == Collider::ShapeType::Sphere) {

        SphereCollider *sphereA = static_cast<SphereCollider *>(colliderA);
        SphereCollider *sphereB = static_cast<SphereCollider *>(colliderB);

        isHit = CollisionMath::IsCollision(sphereA->GetWorldSphere(),
                                           sphereB->GetWorldSphere());
      }
      // ※新しくAABB やOBBなどのコライダーが増えたら、ここに分岐を追加する

      // 当たっていたらお互いのコールバックを呼ぶ
      if (isHit) {
        colliderA->OnCollision(colliderB);
        colliderB->OnCollision(colliderA);
      }
    }
  }
}
