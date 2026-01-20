#include "Core/Game.h"
#include "Core/ResourceObject.h"
#include "Core/SrvManager.h"
#include "Debug/ImGuiManager.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Renderer/ModelRenderer.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/SceneFactory.h"
#include "Scene/SceneManager.h"
#include "Sprite/Sprite.h"
#include "Texture/TextureManager.h"
#include "imgui.h"
#include <cassert>
#include <fstream>

void Game::Initialize() {

  // 基底クラスの初期化処理
  EngineBase::Initialize();

  SceneManager::GetInstance()->Initialize(this);

  sceneFactory_ = new SceneFactory();

  SceneManager::GetInstance()->SetSceneFactory(sceneFactory_);

  SceneManager::GetInstance()->ChangeScene("DEBUG");

  //===========================
  // ローカル変数宣言
  //===========================

  //===========================
  // ImGuiManagerの初期化
  //===========================
  imGuiManager_ = new ImGuiManager();
  imGuiManager_->Initialize(windowSystem_, dx12Core_, srvManager_);

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
  delete imGuiManager_;
  imGuiManager_ = nullptr;

  // シーンの解放
  SceneManager::GetInstance()->Finalize();

  // シーンファクトリー解放
  delete sceneFactory_;

  // 基底クラスの終了処理
  EngineBase::Finalize();
}

void Game::Update() {

  // 基底クラスの更新処理
  EngineBase::Update();
  if (IsEndRequest()) {
    return;
  }

  // ImGui受付開始
  if (imGuiManager_) {
    imGuiManager_->Begin();
  }

  SceneManager::GetInstance()->Update();

  // FPSをセット
  dx12Core_->SetFPS(set60FPS_);

  //=======================
  // アクターの更新
  //=======================

  //=======================
  // デバッグテキストの更新
  //=======================

  // ImGui受付終了
  if (imGuiManager_) {
    imGuiManager_->End();
  }
}

void Game::Draw() {

  EngineBase::BeginFrame();

  // 3D
  // EngineBase::Begin3D();
  SceneManager::GetInstance()->Draw();

  // 2D
  // EngineBase::Begin2D();

  if (imGuiManager_) {
    imGuiManager_->Draw();
  }

  EngineBase::EndFrame();
}
