#include "Core/Game.h"
#include "Collision/CollisionManager.h"
#include "Core/ResourceObject.h"
#include "Core/SrvManager.h"
#include "Framework/ActorManager.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Render/Camera/ICamera.h"
#include "Renderer/ModelRenderer.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/PostProcess.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/SceneFactory.h"
#include "Scene/SceneManager.h"
#include "Sprite/Sprite.h"
#include "Texture/TextureManager.h"
#include <cassert>
#include <fstream>

void Game::Initialize() {

  // 基底クラスの初期化処理
  EngineBase::Initialize();

  SceneManager::GetInstance()->Initialize(this);

  sceneFactory_ = std::make_unique<SceneFactory>();

  SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());

  SceneManager::GetInstance()->ChangeScene("DEBUG");

  //===========================
  // ローカル変数宣言
  //===========================

  //===========================
  // ImGuiManagerの初期化
  //===========================
  imGuiManager_ = std::make_unique<ImGuiManager>();
  imGuiManager_->Initialize(windowSystem_.get(), dx12Core_.get(),
                            srvManager_.get());

  //===========================
  // RenderPipelineの初期化
  //===========================
  renderPipeline_ = std::make_unique<RenderPipeline>();
  renderPipeline_->Initialize(dx12Core_.get(), srvManager_.get());

  // texture切り替え用
  bool useMonsterBall = true;

  // Lighting
  bool enableLighting = true;

  // デルタタイム 一旦60fpsで固定
  const float kDeltaTime = 1.0f / 60.0f;

  bool isUpdate = false;

  bool useBillboard = true;

  //// 風を出す範囲 ※今は使用していないのでコメントアウト
  // AccelerationField accelerationField;
  // accelerationField.acceleration = {15.0f, 0.0f, 0.0f};
  // accelerationField.area.min = {-1.0f, -1.0f, -1.0f};
  // accelerationField.area.max = {1.0f, 1.0f, 1.0f};
}

void Game::Finalize() {

  imGuiManager_->Finalize();

  // シーンの解放
  SceneManager::GetInstance()->Finalize();

  // マネージャーのメモリ解放
  ActorManager::GetInstance()->Clear();
  CollisionManager::GetInstance()->Clear();

  renderPipeline_.reset();
  imGuiManager_.reset();
  sceneFactory_.reset();

  // 基底クラスの終了処理
  EngineBase::Finalize();
}

void Game::Update() {

  // 基底クラスの更新処理
  EngineBase::Update();
  if (IsEndRequest()) {
    return;
  }

#ifdef USE_IMGUI

  // ImGui受付開始
  if (imGuiManager_) {
    imGuiManager_->Begin();
  }

#endif // USE_IMGUI

  SceneManager::GetInstance()->Update();

  // FPSをセット
  dx12Core_->SetFPS(set60FPS_);

  //=======================
  // アクターの更新
  //=======================

  //=======================
  // デバッグテキストの更新
  //=======================

#ifdef USE_IMGUI

  // デモウィンドウ表示ON
  ImGui::ShowDemoWindow();

  // ImGui受付終了
  if (imGuiManager_) {
    imGuiManager_->End();
  }

#endif // USE_IMGUI
}

void Game::Draw() {

  EngineBase::BeginFrame();

  // HDRキャンバスの準備をして、3D空間を描画
  renderPipeline_->Begin3DPass(dx12Core_.get());
  SceneManager::GetInstance()->Draw();

  // プロジェクション逆行列のセット（PostProcessが必要とするため）
  auto pp = SceneManager::GetInstance()->GetCurrentScenePostProcess();
  if (pp) {
    const ICamera *defaultCamera = object3dRenderer_->GetDefaultCamera();
    if (defaultCamera) {
      pp->SetProjectionInverse(Inverse(defaultCamera->GetProjectionMatrix()));
    }
  }

  // ポストプロセスを計算し、メイン画面に焼き付ける
  renderPipeline_->DrawPostProcess(dx12Core_.get(), srvManager_.get(), pp);

  // 2Dオーバーレイ描画パス（ポストプロセスの後に上書き描画する）
  SceneManager::GetInstance()->DrawOverlay();

  // 2D
  // EngineBase::Begin2D();

  if (imGuiManager_) {
    imGuiManager_->Draw();
  }

  EngineBase::EndFrame();
}
