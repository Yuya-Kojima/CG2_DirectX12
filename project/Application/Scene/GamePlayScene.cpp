#include "GamePlayScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Debug/ImGuiManager.h"
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

void GamePlayScene::Initialize(EngineBase *engine) {

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

  sprite_ = std::make_unique<Sprite>();
  sprite_->Initialize(engine_->GetSpriteRenderer(), "resources/white1x1.png");

  spritePosition_ = {
      100.0f,
      100.0f,
  };

  sprite_->SetPosition(spritePosition_);

  uvTransformSprite_ = {
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  //===========================
  // 3Dオブジェクト関係の初期化
  //===========================

  // カメラの生成と初期化
  camera_ = std::make_unique<GameCamera>();
  camera_->SetRotate({0.3f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 4.0f, -10.0f});
  engine_->GetObject3dRenderer()->SetDefaultCamera(camera_.get());

  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f},
      {0.3f, 0.0f, 0.0f},
      {0.0f, 4.0f, -10.0f},
  };

  // デバッグカメラ
  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize({0.0f, 4.0f, -10.0f});

  // モデルの読み込み

  // オブジェクトの生成と初期化

  //===========================
  // パーティクル関係の初期化
  //===========================
}

void GamePlayScene::Finalize() {}

void GamePlayScene::Update() {

  // Sound更新
  SoundManager::GetInstance()->Update();

  // タイトルシーンへ移行
  if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
    SceneManager::GetInstance()->ChangeScene("DEBUG");
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

#ifdef USE_IMGUI
  //========================
  // Sprite座標をImGuiで操作
  //========================
  ImGui::SetNextWindowSize(ImVec2(500.0f, 100.0f), ImGuiCond_Once);

  ImGui::Begin("Sprite Pos");

  ImGui::SliderFloat2("position", &spritePosition_.x, 0.0f, 1280.0f, "%.1f");

  ImGui::End();
#endif

  sprite_->SetPosition(spritePosition_);
  sprite_->SetSize({100.0f, 100.0f});
  sprite_->Update(uvTransformSprite_);

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

void GamePlayScene::Draw() {
  Draw3D();
  Draw2D();
}

void GamePlayScene::Draw3D() {
  engine_->Begin3D();

  // ここから下で3DオブジェクトのDrawを呼ぶ
}

void GamePlayScene::Draw2D() {
  engine_->Begin2D();

  // ここから下で2DオブジェクトのDrawを呼ぶ
  sprite_->Draw();
}