#include "Actor/Enemy.h"
#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Render/Object3d/Object3d.h"
#include <Windows.h> // OutputDebugStringA用

Enemy::Enemy() = default;

Enemy::~Enemy() {
  if (collider_) {
    CollisionManager::GetInstance()->Remove(collider_.get());
  }
}

void Enemy::Initialize() {
  // 敵のコライダーの初期化
  collider_ = std::make_unique<SphereCollider>(this);
  collider_->SetRadius(1.5f); // 敵の当たり判定の大きさを少し大きめに設定
  collider_->SetAttribute(kCollisionAttributeEnemy); // 自分は「敵」
  collider_->SetMask(kCollisionAttributePlayer |
                     kCollisionAttributePlayerBullet); // 自機や自機の弾と当たる
  CollisionManager::GetInstance()->Register(collider_.get());
}

void Enemy::Update() {
  // モデルが存在していれば、敵の座標をモデルに反映して更新
  if (model_) {
    model_->SetTranslation(transform_.translate);
    model_->SetRotation(transform_.rotate);
    model_->SetScale(transform_.scale);
    model_->Update();
  }
}

void Enemy::Draw3D() {
  // 3Dモデルの描画
  if (model_) {
    model_->Draw();
  }
}

void Enemy::OnCollision(Collider *other) {
  // ぶつかった時にデバッグ出力
  OutputDebugStringA("========================\n");
  OutputDebugStringA("Enemy hit something!\n");
  OutputDebugStringA("========================\n");

  // ぶつかったら自分を破壊
  Destroy();
}
