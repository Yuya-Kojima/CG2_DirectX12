#include "LockOn.h"
#include "Math/MathUtil.h"
#include "Render/Object3d/Object3d.h"

void LockOn::Initialize(SpriteRenderer *spriteRenderer) {
  // レティクル（カーソル）用のスプライトを生成・初期化
  reticle_ = std::make_unique<Sprite>();
  reticle_->Initialize(spriteRenderer, "resources/white1x1.png");

  // カーソルの見た目設定
  reticle_->SetSize({50.0f, 50.0f});
  reticle_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤色

  // 初期状態はターゲットなし
  target_ = nullptr;
}

void LockOn::Update(const std::vector<Object3d *> &enemies,
                    const Matrix4x4 &viewProjectionMatrix) {

  // --------------------------------------------------
  //  ロックオン対象を探す処理
  // --------------------------------------------------
  if (!enemies.empty()) {
    target_ = enemies[0];
  } else {
    target_ = nullptr;
  }

  // --------------------------------------------------
  // ターゲットがいる場合、カーソルを追従させる処理
  // --------------------------------------------------
  if (target_) {
    // 敵の3D座標を取得
    Vector3 targetPos = target_->GetTranslation();

    // 2D画面座標へ変換
    Vector2 screenPos =
        WorldToScreen(targetPos, viewProjectionMatrix, 1280.0f, 720.0f);

    // カーソルが敵の中心にくるようにサイズ分ずらす
    Vector2 drawPos;
    drawPos.x = screenPos.x - 25.0f; // サイズの半分
    drawPos.y = screenPos.y - 25.0f;

    reticle_->SetPosition(drawPos);

    // スプライトの行列更新
    Transform defaultUV;
    defaultUV.scale = {1.0f, 1.0f, 1.0f};
    defaultUV.rotate = {0.0f, 0.0f, 0.0f};
    defaultUV.translate = {0.0f, 0.0f, 0.0f};
    reticle_->Update(defaultUV);
  }
}

void LockOn::Draw() {
  // ターゲットがいる時だけカーソルを描画する(仮)
  if (target_ && reticle_) {
    reticle_->Draw();
  }
}
