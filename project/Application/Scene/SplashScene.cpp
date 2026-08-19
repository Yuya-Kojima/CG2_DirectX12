#include "SplashScene.h"
#include "Scene/SceneManager.h"
#include "Sprite/Sprite.h"
#include "Renderer/SpriteRenderer.h"

void SplashScene::Initialize(EngineBase *engine) {
  BaseScene::Initialize(engine);
  engine_ = engine;

  // ロゴ用のスプライトを生成
  // プロジェクト内に存在する適当なテクスチャを指定します (必要なら変更可能)
  logoSprite_ = std::make_unique<Sprite>();
  logoSprite_->Initialize(engine_->GetSpriteRenderer(), "resources/sample_logo_transparent.png");

  time_ = 0.0f;
  transitionTimer_ = 0.0f;
  isTransitioning_ = false;
}

void SplashScene::Finalize() {
  logoSprite_.reset();
}

void SplashScene::Update() {
  const float dt = 1.0f / 60.0f;
  time_ += dt;
  transitionTimer_ += dt;

  // 画面中央に配置
  logoSprite_->SetPosition({640.0f, 360.0f});
  // アスペクト比を維持するため、SetSizeで上書きせず、SetScaleで倍率を指定
  logoSprite_->SetScale({0.9f, 0.9f}); 
  logoSprite_->SetAnchorPoint({0.5f, 0.5f}); // 中心を基準にする

  Transform uvTransform = {
      {1.0f, 1.0f, 1.0f}, // scale
      {0.0f, 0.0f, 0.0f}, // rotation
      {0.0f, 0.0f, 0.0f}  // translation
  };
  logoSprite_->Update(uvTransform);

  if (!isTransitioning_ && transitionTimer_ >= displayDuration_) {
    // 時間が来たらDebugSceneへフェードアウト
    SceneManager::GetInstance()->SetNextTransitionFade(1.0f);
    SceneManager::GetInstance()->ChangeScene("DEBUG");
    isTransitioning_ = true;
  }
}

void SplashScene::Draw() {
  // SplashSceneは3Dを描画しない
}

void SplashScene::Draw3D() {
}

void SplashScene::Draw2D() {
  // 描画前にAIエフェクト用のパイプラインステート（シェーダーなど）をセットする
  engine_->GetSpriteRenderer()->BeginAiEffect();
  
  // AI生成エフェクト専用の描画メソッドを呼び出し、時間を渡す
  logoSprite_->DrawAiEffect(time_);
}
