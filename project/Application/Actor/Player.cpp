#include "Actor/Player.h"
#include "Actor/LockOn.h"
#include "Camera/ICamera.h"
#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include <Windows.h>

Player::Player() = default;
Player::~Player() {
  if (collider_) {
    CollisionManager::GetInstance()->Remove(collider_.get());
  }
}

void Player::Initialize() {
  // プレイヤー自身の初期化処理

  // ロックオン機能の生成と初期化
  if (spriteRenderer_) {
    lockOn_ = std::make_unique<LockOn>();
    lockOn_->Initialize(spriteRenderer_);
  }

  // コライダーの初期化
  collider_ = std::make_unique<SphereCollider>(this);
  collider_->SetRadius(1.0f); // プレイヤーの当たり判定の大きさ
  collider_->SetAttribute(kCollisionAttributePlayer); // 自分は「自機」
  collider_->SetMask(kCollisionAttributeEnemy |
                     kCollisionAttributeEnemyBullet); // 敵や敵の弾と当たる
  CollisionManager::GetInstance()->Register(collider_.get());
}

void Player::Update() {
  // プレイヤー自身の更新処理
  // ロックオンの更新処理
  if (lockOn_ && camera_ && target_) {
    std::vector<Object3d *> enemies = {target_};
    lockOn_->Update(enemies, camera_->GetViewProjectionMatrix());
  }
}

void Player::Draw3D() {
  // プレイヤー自身の描画処理
}

void Player::Draw2D() {
  // カーソルの描画
  if (lockOn_) {
    lockOn_->Draw();
  }
}

void Player::OnCollision(Collider *other) {
  // ぶつかった時にデバッグ出力
  OutputDebugStringA("========================\n");
  OutputDebugStringA("Player hit by something!\n");
  OutputDebugStringA("========================\n");
}
