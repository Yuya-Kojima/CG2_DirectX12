#include "LockOn.h"
#include "Math/MathUtil.h"
#include "Actor/Enemy.h"
#include "Render/Object3d/Object3d.h"

void LockOn::Initialize(SpriteRenderer *spriteRenderer) {
  // レティクル（カーソル）用のスプライトを最大数分、生成・初期化
  for (int i = 0; i < kMaxLockOnCount; ++i) {
    reticles_[i] = std::make_unique<Sprite>();
    reticles_[i]->Initialize(spriteRenderer, "resources/white1x1.png");
    reticles_[i]->SetSize({50.0f, 50.0f});
    reticles_[i]->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤色
  }

  // 初期状態はターゲットリストを空にする
  targets_.clear();
}

void LockOn::Update(const std::vector<Enemy *> &enemies,
                    const Matrix4x4 &viewProjectionMatrix,
                    const Vector2 &reticlePos,
                    bool isLockOnMode) {
  
  viewProjectionMatrix_ = viewProjectionMatrix; // 描画用にキャッシュ

  // --------------------------------------------------
  //  ロックオン対象を探す処理
  // --------------------------------------------------
  const float lockOnRadius = 100.0f; // ロックオン可能な照準からの半径

  // ロックオンモード（長押し中）でのみ、新たな敵をストックする
  if (isLockOnMode && targets_.size() < kMaxLockOnCount) {
    for (Enemy* enemy : enemies) {
      if (!enemy) continue;

      // 撃破されて消滅している敵はロックオン対象から外す
      if (enemy->IsDead()) continue;

      // すでにロックオン済みの敵は無視
      if (std::find(targets_.begin(), targets_.end(), enemy) != targets_.end()) {
        continue;
      }

      Vector3 worldPos = enemy->GetTransform().translate;
      Vector2 screenPos = WorldToScreen(worldPos, viewProjectionMatrix, 1280.0f, 720.0f);

      // 照準と敵の画面上の距離を計算
      float dx = screenPos.x - reticlePos.x;
      float dy = screenPos.y - reticlePos.y;
      float dist = std::sqrt(dx * dx + dy * dy);

      if (dist <= lockOnRadius) {
        targets_.push_back(enemy); // ロックオンストックに追加
        break; // 1フレームに1体ずつロックオンする
      }
    }
  }
}

void LockOn::Draw() {
  for (size_t i = 0; i < targets_.size(); ++i) {
    Enemy* target = targets_[i];
    if (!target) continue;
    
    Vector3 targetPos = target->GetTransform().translate;
    Vector2 screenPos = WorldToScreen(targetPos, viewProjectionMatrix_, 1280.0f, 720.0f);

    Vector2 drawPos;
    drawPos.x = screenPos.x - 25.0f; // サイズの半分
    drawPos.y = screenPos.y - 25.0f;

    reticles_[i]->SetPosition(drawPos);
    Transform defaultUV;
    defaultUV.scale = {1.0f, 1.0f, 1.0f};
    defaultUV.rotate = {0.0f, 0.0f, 0.0f};
    defaultUV.translate = {0.0f, 0.0f, 0.0f};
    reticles_[i]->Update(defaultUV);
    
    reticles_[i]->Draw(); 
  }
}
