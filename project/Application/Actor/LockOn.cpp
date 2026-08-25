#include "LockOn.h"
#include "Math/MathUtil.h"
#include "Framework/BaseActor.h"
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
  lockOnDelayTimer_ = 0;
}

void LockOn::Update(const std::vector<BaseActor *> &inputTargets,
                    const Matrix4x4 &viewProjectionMatrix,
                    const Vector2 &reticlePos,
                    bool isLockOnMode, float lockOnRadius) {
  
  viewProjectionMatrix_ = viewProjectionMatrix; // 描画用にキャッシュ

  // --------------------------------------------------
  //  ロックオン対象を探す処理
  // --------------------------------------------------

  if (lockOnDelayTimer_ > 0) {
    lockOnDelayTimer_--;
  }

  // ロックオンモード（長押し中）かつ、ディレイが明けている場合のみ新たな敵をストックする
  if (isLockOnMode && targets_.size() < kMaxLockOnCount && lockOnDelayTimer_ <= 0) {
    for (BaseActor* target : inputTargets) {
      if (!target) continue;

      // 撃破されて消滅している敵はロックオン対象から外す
      if (target->IsDead()) continue;

      // すでにロックオン済みの敵は無視
      if (std::find(targets_.begin(), targets_.end(), target) != targets_.end()) {
        continue;
      }

      Vector3 worldPos = target->GetTransform().translate;
      Vector2 screenPos = WorldToScreen(worldPos, viewProjectionMatrix, 1280.0f, 720.0f);

      // 照準と敵の画面上の距離を計算
      float dx = screenPos.x - reticlePos.x;
      float dy = screenPos.y - reticlePos.y;
      float dist = std::sqrt(dx * dx + dy * dy);

      if (dist <= lockOnRadius) {
        targets_.push_back(target); // ロックオンストックに追加
        lockOnDelayTimer_ = kLockOnInterval; // ディレイを開始
        break; // 1フレームに1体ずつロックオンする
      }
    }
  }
}

void LockOn::Draw() {
  for (size_t i = 0; i < targets_.size(); ++i) {
    BaseActor* target = targets_[i];
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
