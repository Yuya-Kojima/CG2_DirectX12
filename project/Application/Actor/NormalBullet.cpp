#include "NormalBullet.h"
#include "Render/Object3d/Object3d.h"
#include "Math/MathUtil.h"
#include <Windows.h>
#include "Actor/Enemy.h"
#include "Render/Renderer/LineRenderer.h"
#include <cmath>

NormalBullet::NormalBullet() {}
NormalBullet::~NormalBullet() {}

void NormalBullet::Initialize(Object3dRenderer* renderer, const Vector3& startPos, const Vector3& velocity, const std::vector<Enemy*>& enemies) {
  object3d_ = std::make_unique<Object3d>();
  object3d_->Initialize(renderer);
  
  // 通常弾のモデル
  object3d_->SetModel("suzanne.obj"); 
  object3d_->SetScale({2.0f, 2.0f, 2.0f}); 
  object3d_->SetColor({1.0f, 0.5f, 0.0f, 1.0f}); 
  object3d_->SetTranslation(startPos);

  velocity_ = velocity; // 目標へのベクトル
  enemies_ = enemies;
  lifeTimer_ = 180; 
}

void NormalBullet::Update() {
  if (isDead_) return;

  lifeTimer_--;
  if (lifeTimer_ <= 0) {
    isDead_ = true;
  }

  // 座標を更新（ホーミングせず直進のみ）
  Vector3 pos = object3d_->GetTranslation();
  pos.x += velocity_.x;
  pos.y += velocity_.y;
  pos.z += velocity_.z;
  object3d_->SetTranslation(pos);

  // 敵への当たり判定（簡易的に距離で判定）
  for (Enemy* enemy : enemies_) {
    if (!enemy) continue;
    if (enemy->IsDead()) continue; // 既に死んでいる敵は無視

    Vector3 enemyPos = enemy->GetTransform().translate;
    
    // トンネリングを防ぐため、1フレーム前からの線分と敵の距離を計算する
    Vector3 prevPos = {pos.x - velocity_.x, pos.y - velocity_.y, pos.z - velocity_.z};
    Vector3 lineDir = velocity_;
    float lineLen = Length(lineDir);
    if (lineLen > 0.001f) {
      lineDir.x /= lineLen; lineDir.y /= lineLen; lineDir.z /= lineLen;
    }
    
    Vector3 toEnemy = {enemyPos.x - prevPos.x, enemyPos.y - prevPos.y, enemyPos.z - prevPos.z};
    float t = Dot(toEnemy, lineDir);
    // 線分上にクランプ 
    t = (std::max)(0.0f, (std::min)(lineLen, t)); 
    
    Vector3 closestPoint = {prevPos.x + lineDir.x * t, prevPos.y + lineDir.y * t, prevPos.z + lineDir.z * t};
    Vector3 diff = {enemyPos.x - closestPoint.x, enemyPos.y - closestPoint.y, enemyPos.z - closestPoint.z};
    
    float dist = Length(diff);

    // 弾の半径
    float bulletRadius = 2.5f;
    
    // 敵のスケールを考慮した実際の半径を計算
    Vector3 enemyScale = enemy->GetTransform().scale;
    float maxScale = (std::max)(enemyScale.x, (std::max)(enemyScale.y, enemyScale.z));
    float enemyRadius = 1.5f * maxScale;
    
    // ヒット判定
    if (dist < bulletRadius + enemyRadius) {
      isDead_ = true; // 弾が消える
      enemy->TakeDamage(1); // 敵に1ダメージ与える
      OutputDebugStringA("Normal Bullet Hit!\n");
      break;
    }
  }

  object3d_->Update();
}

void NormalBullet::Draw3D() {
  if (!isDead_ && object3d_) {
    object3d_->Draw();

#ifdef USE_IMGUI
    // ==== デバッグ描画 ====
    LineRenderer* lineRenderer = LineRenderer::GetInstance();
    int segments = 16;
    float angleStep = 2.0f * 3.14159265f / segments;
    Vector4 color = {0.0f, 0.0f, 1.0f, 1.0f}; 
    float radius = 2.5f;
    Vector3 pos = object3d_->GetTranslation();

    for (int i = 0; i < segments; ++i) {
      float angle1 = i * angleStep;
      float angle2 = (i + 1) * angleStep;

      // XY plane
      Vector3 p1_xy = {pos.x + std::cos(angle1) * radius, pos.y + std::sin(angle1) * radius, pos.z};
      Vector3 p2_xy = {pos.x + std::cos(angle2) * radius, pos.y + std::sin(angle2) * radius, pos.z};
      lineRenderer->DrawLine(p1_xy, p2_xy, color);

      // XZ plane
      Vector3 p1_xz = {pos.x + std::cos(angle1) * radius, pos.y, pos.z + std::sin(angle1) * radius};
      Vector3 p2_xz = {pos.x + std::cos(angle2) * radius, pos.y, pos.z + std::sin(angle2) * radius};
      lineRenderer->DrawLine(p1_xz, p2_xz, color);

      // YZ plane
      Vector3 p1_yz = {pos.x, pos.y + std::cos(angle1) * radius, pos.z + std::sin(angle1) * radius};
      Vector3 p2_yz = {pos.x, pos.y + std::cos(angle2) * radius, pos.z + std::sin(angle2) * radius};
      lineRenderer->DrawLine(p1_yz, p2_yz, color);
    }
#endif
  }
}
