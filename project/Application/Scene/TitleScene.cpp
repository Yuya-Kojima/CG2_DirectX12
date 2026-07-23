#include "TitleScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Framework/GameManager.h"
#include "Framework/UIManager.h"
#include "Input/InputKeyState.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/SceneManager.h"
#include "Sprite/Sprite.h"
#include "Texture/TextureManager.h"
#include "Renderer/PostProcess.h"

// ImGuiを使用するためのインクルード
#ifdef USE_IMGUI
#include "Debug/ImGuiManager.h"
#endif

void TitleScene::Initialize(EngineBase *engine) {

  // 基底クラスの初期化（PostProcessの生成など）
  BaseScene::Initialize(engine);

  // 参照をコピー
  engine_ = engine;

  //===========================
  // テクスチャファイルの読み込み
  //===========================

  //===========================
  // オーディオファイルの読み込み
  //===========================

  //===========================
  // スプライト関係の初期化
  //===========================
  logoSprite_ = std::make_unique<Sprite>();
  logoSprite_->Initialize(engine_->GetSpriteRenderer(), "resources/sample_logo.png");
  logoSprite_->SetPosition({640.0f, 360.0f});
  logoSprite_->SetAnchorPoint({0.5f, 0.5f});
  Vector2 originalSize = logoSprite_->GetSize();
  logoSprite_->SetSize({originalSize.x * 0.6f, originalSize.y * 0.6f});
  logoSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});

  blackSprite_ = std::make_unique<Sprite>();
  blackSprite_->Initialize(engine_->GetSpriteRenderer(), "resources/white1x1.png");
  blackSprite_->SetSize({1280.0f, 720.0f});
  blackSprite_->SetPosition({640.0f, 360.0f});
  blackSprite_->SetAnchorPoint({0.5f, 0.5f});
  blackSprite_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});

  //===========================
  // 3Dオブジェクト関係の初期化
  //===========================

  // カメラの生成と初期化
  camera_ = std::make_unique<GameCamera>();
  camera_->SetRotate({0.3f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 4.0f, -10.0f});

  // デバッグカメラ
  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize({0.0f, 4.0f, -10.0f});

  // デフォルトカメラのセット
  engine_->GetObject3dRenderer()->SetDefaultCamera(camera_.get());

  // モデルの読み込み

  // オブジェクトの生成と初期化

  //===========================
  // パーティクル関係の初期化
  //===========================

  // UIの読み込み
  UIManager::GetInstance()->Load("resources/UI/TitleUI.json");
}

void TitleScene::Finalize() {}

void TitleScene::Update() {

  // Sound更新
  SoundManager::GetInstance()->Update();

  // ステージセレクトシーンへ移行
  if (GameManager::GetInstance()->IsGlobalPlayMode()) {
    if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
      SceneManager::GetInstance()->SetNextTransitionFade(0.5f);
      SceneManager::GetInstance()->ChangeScene("STAGE_SELECT");
    }
  }

  // デバッグカメラ切り替え
  if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
    if (useDebugCamera_) {
      useDebugCamera_ = false;
    } else {
      useDebugCamera_ = true;
    }
  }

  //=======================
  // スプライトの更新
  //=======================
  Input *input = GameManager::GetInstance()->IsGlobalPlayMode()
                     ? engine_->GetInputManager()
                     : nullptr;
  UIManager::GetInstance()->Update(input);

  // --- Splash Screen Logic (Two-stage fade) ---
  splashTimer_ += 1.0f / 60.0f; // 毎フレーム加算
  
  float logoAlpha = 0.0f;
  if (splashTimer_ >= 0.5f && splashTimer_ < 1.5f) {
    logoAlpha = (splashTimer_ - 0.5f) / 1.0f; // ロゴフェードイン
  } else if (splashTimer_ >= 1.5f && splashTimer_ < 2.5f) {
    logoAlpha = 1.0f; // 完全表示
  } else if (splashTimer_ >= 2.5f && splashTimer_ < 3.5f) {
    logoAlpha = 1.0f - (splashTimer_ - 2.5f) / 1.0f; // ロゴフェードアウト
  }

  float blackAlpha = 1.0f;
  if (splashTimer_ >= 3.5f && splashTimer_ < 4.5f) {
    blackAlpha = 1.0f - (splashTimer_ - 3.5f) / 1.0f; // ロゴが消えた後、黒背景フェードアウト
  } else if (splashTimer_ >= 4.5f) {
    blackAlpha = 0.0f;
  }

  if (blackSprite_) {
    blackSprite_->SetColor({0.0f, 0.0f, 0.0f, blackAlpha});
    Transform blackTransform;
    blackTransform.translate = {0,0,0};
    blackTransform.rotate = {0,0,0};
    blackTransform.scale = {1,1,1};
    blackSprite_->Update(blackTransform);
  }

  if (logoSprite_) {
    logoSprite_->SetColor({1.0f, 1.0f, 1.0f, logoAlpha});
    Transform spriteTransform;
    spriteTransform.translate = {0,0,0};
    spriteTransform.rotate = {0,0,0};
    spriteTransform.scale = {1.0f, 1.0f, 1.0f}; // UV scale is normal
    logoSprite_->Update(spriteTransform);
  }

  // --- Post Process AI Effect ---
  // ロゴ表示中（2.0秒）に一番エフェクトが強くなるようにする
  float effectStrength = 0.0f;
  if (splashTimer_ < 4.0f) {
    effectStrength = sinf((splashTimer_ / 4.0f) * 3.14159f);
    if (effectStrength < 0.0f) effectStrength = 0.0f;
  }
  
  GetPostProcess()->SetPostEffectType(11); // Custom AI Effect ID changed to 11
  GetPostProcess()->SetAiIntensity(0.311f * effectStrength);
  GetPostProcess()->SetAiSpeed(0.806f);
  GetPostProcess()->SetAiAberration(0.006f * effectStrength);
  GetPostProcess()->SetAiColor(0.0f, 0.0f, 0.0f);
  GetPostProcess()->SetTime(splashTimer_);

  //=======================
  // 3Dオブジェクトの更新
  //=======================

  //=======================
  // カメラの更新
  //=======================
  const ICamera *activeCamera = nullptr;

  if (useDebugCamera_) {
    debugCamera_->Update(*engine_->GetInputManager());
    activeCamera = debugCamera_->GetCamera();
  } else {
    camera_->Update();
    activeCamera = camera_.get();
  }

  // アクティブカメラを描画で使用する
  engine_->GetObject3dRenderer()->SetDefaultCamera(activeCamera);
}

void TitleScene::Draw() { Draw3D(); }

void TitleScene::Draw3D() {
  engine_->Begin3D();

  // ここから下で3DオブジェクトのDrawを呼ぶ

  // スプライトをメインキャンバスに描画してポストエフェクトをかける
  engine_->Begin2D();
  if (blackSprite_) {
    blackSprite_->Draw();
  }
  if (logoSprite_) {
    logoSprite_->Draw();
  }
  engine_->End2D();
}

void TitleScene::Draw2D() {
  // スプラッシュ画面（ロゴフェード）が完了するまではUI（タイトル文字等）を描画しない
  if (splashTimer_ >= 4.5f) {
    // ここから下で2DオブジェクトのDrawを呼ぶ
    UIManager::GetInstance()->Draw();
  }
}