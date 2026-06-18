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
}

void TitleScene::Draw2D() {
  // ここから下で2DオブジェクトのDrawを呼ぶ
  UIManager::GetInstance()->Draw();
}