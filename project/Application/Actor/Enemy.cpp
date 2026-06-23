#include "Actor/Enemy.h"
#include "Camera/ICamera.h"
#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Math/MathUtil.h"
#include "Render/Object3d/Object3d.h"
#include "Render/Particle/IParticleEmitter.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleManager.h"
#include <Windows.h> // OutputDebugStringA用
#include <cmath>

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
  // 死んでいる場合は当たり判定を消して何もさせない
  if (isDead_) {
    if (collider_) {
      CollisionManager::GetInstance()->Remove(collider_.get());
      collider_.reset(); // コライダーを破棄
    }
    return;
  }

  aliveTime_ += 1.0f / 60.0f; // 簡易的に60FPS固定で時間計算

  if (camera_) {
    Matrix4x4 viewMatrix = camera_->GetViewMatrix();
    Matrix4x4 cameraWorld = Inverse(viewMatrix);
    Vector3 cameraPos = {cameraWorld.m[3][0], cameraWorld.m[3][1],
                         cameraWorld.m[3][2]};
    Vector3 cameraRight = {cameraWorld.m[0][0], cameraWorld.m[0][1],
                           cameraWorld.m[0][2]};
    Vector3 cameraUp = {cameraWorld.m[1][0], cameraWorld.m[1][1],
                        cameraWorld.m[1][2]};
    Vector3 cameraForward = {cameraWorld.m[2][0], cameraWorld.m[2][1],
                             cameraWorld.m[2][2]};

    if (moveType_ != MoveType::Stationary) {
      float currentXOffset = spawnOffset_.x;
      float currentYOffset = spawnOffset_.y;
      float currentZOffset = spawnOffset_.z;

      if (moveType_ == MoveType::Straight) {
        currentZOffset -= speed_ * aliveTime_ * 60.0f; // 奥から手前
      } else if (moveType_ == MoveType::Parallel) {
        // オフセットのまま（追従）
      } else if (moveType_ == MoveType::SineWave) {
        currentZOffset -= speed_ * aliveTime_ * 60.0f; // 奥から手前へつつ
        currentXOffset += std::sin(aliveTime_ * 5.0f) * 20.0f; // 左右に波打つ
      }

      transform_.translate =
          cameraPos +
          Vector3{cameraRight.x * currentXOffset,
                  cameraRight.y * currentXOffset,
                  cameraRight.z * currentXOffset} +
          Vector3{cameraUp.x * currentYOffset, cameraUp.y * currentYOffset,
                  cameraUp.z * currentYOffset} +
          Vector3{cameraForward.x * currentZOffset,
                  cameraForward.y * currentZOffset,
                  cameraForward.z * currentZOffset};
    }
  } else {
    // カメラ情報がない場合のフォールバック
    transform_.translate.x += moveDirection_.x * speed_;
    transform_.translate.y += moveDirection_.y * speed_;
    transform_.translate.z += moveDirection_.z * speed_;
  }

  // 被弾時の点滅処理（赤色にする）
  if (hitFlashTimer_ > 0) {
    hitFlashTimer_--;
    if (model_) {
      model_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤色
    }
  } else {
    if (model_) {
      model_->SetColor(baseColor_); // 元の色
    }
  }

  UpdateTransform();
}

void Enemy::UpdateTransform() {
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

  // ここでは即死させず、TakeDamageは弾の側から呼ぶようにする
}

void Enemy::TakeDamage(int damage) {
  if (isDead_) {
    return;
  }

  hp_ -= damage;
  hitFlashTimer_ = 5; // 5フレーム赤く点滅

  if (hp_ <= 0) {
    OutputDebugStringA("Enemy Destroyed!\n");
    
    if (onDestroyedCallback_) {
      onDestroyedCallback_();
    }
    Destroy();
  }
}
