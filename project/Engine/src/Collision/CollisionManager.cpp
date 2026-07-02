#include "Collision/CollisionManager.h"
#include "Collision/Collider.h"
#include "Collision/SphereCollider.h"
#include "Math/CollisionMath.h"
#include "Math/MathUtil.h"

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
  // 全コライダーの座標を更新
  for (Collider *collider : colliders_) {
    collider->Update();
  }

  // 総当り判定
  CheckAllCollisions();
}

void CollisionManager::DrawDebug() {
  for (Collider *collider : colliders_) {
    collider->DrawDebug();
  }
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

        const Vector3& velA = sphereA->GetVelocity();
        const Vector3& velB = sphereB->GetVelocity();

        if (LengthSq(velA) > 0.0f || LengthSq(velB) > 0.0f) {
            // どちらかが速度を持っている場合、Swept Sphere（線分×球）判定を行う
            // ここでは簡易的に、速度を持つ側を「線分」とし、持たない側を「球」とする。
            // 双方が速度を持つ場合は相対速度を計算すべきだが、現状の弾丸は敵に対して圧倒的に速いため簡略化する。
            SphereCollider* movingSphere = LengthSq(velA) > LengthSq(velB) ? sphereA : sphereB;
            SphereCollider* targetSphere = movingSphere == sphereA ? sphereB : sphereA;

            Segment seg;
            const Vector3& vel = movingSphere->GetVelocity();
            Vector3 currentPos = movingSphere->GetWorldSphere().center;
            // 1フレーム前の位置から今の位置までの線分
            seg.origin = Subtract(currentPos, vel);
            seg.diff = vel;

            Sphere expandedSphere = targetSphere->GetWorldSphere();
            // 移動する球の半径を加算して太さを考慮する
            expandedSphere.radius += movingSphere->GetWorldSphere().radius;

            isHit = CollisionMath::IsCollision(seg, expandedSphere);
        } else {
            // 静的（離散的）な球×球判定
            isHit = CollisionMath::IsCollision(sphereA->GetWorldSphere(),
                                               sphereB->GetWorldSphere());
        }
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

bool CollisionManager::Raycast(const Ray& ray, uint32_t mask, Collider** outCollider, float* outDistance) {
  bool hit = false;
  float closestDist = 1e20f;
  Collider* closestCollider = nullptr;

  for (Collider* collider : colliders_) {
    // マスクでフィルタリング
    if (!(collider->GetAttribute() & mask)) {
      continue;
    }

    if (collider->GetShapeType() == Collider::ShapeType::Sphere) {
      SphereCollider* sphere = static_cast<SphereCollider*>(collider);
      float dist = 0.0f;
      if (IsCollision(ray, sphere->GetWorldSphere(), &dist)) {
        if (dist < closestDist) {
          closestDist = dist;
          closestCollider = collider;
          hit = true;
        }
      }
    }
  }

  if (hit) {
    if (outCollider) *outCollider = closestCollider;
    if (outDistance) *outDistance = closestDist;
    return true;
  }

  return false;
}
