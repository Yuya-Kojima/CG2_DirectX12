#include "Core/Game.h"
#include "Core/ResourceObject.h"
#include "Core/SrvManager.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Renderer/ModelRenderer.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Renderer/PostProcess.h"
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

  // RenderTextureのSRV作成 (SrvManagerを使って)
  renderTextureSrvIndex_ = srvManager_->Allocate();
  D3D12_SHADER_RESOURCE_VIEW_DESC renderTextureSrvDesc{};
  renderTextureSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  renderTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  renderTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  renderTextureSrvDesc.Texture2D.MipLevels = 1;

  dx12Core_->GetDevice()->CreateShaderResourceView(
      dx12Core_->GetRenderTextureResource(), &renderTextureSrvDesc,
      srvManager_->GetCPUDescriptorHandle(renderTextureSrvIndex_));

  // PostProcessの初期化
  postProcess_ = std::make_unique<PostProcess>();
  postProcess_->Initialize(dx12Core_.get());

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

  ImGui::Begin("Settings");
  static int monotoneType = 0; // 0: Normal, 1: Grayscale, 2: Sepia, 3: Custom
  static float monotoneColor[3] = {1.0f, 1.0f, 1.0f};

  const char* items[] = { "Normal", "Grayscale", "Sepia", "Custom" };
  if (ImGui::Combo("Monotone Preset", &monotoneType, items, IM_ARRAYSIZE(items))) {
      if (monotoneType == 1) { // Grayscale
          monotoneColor[0] = 1.0f;
          monotoneColor[1] = 1.0f;
          monotoneColor[2] = 1.0f;
      } else if (monotoneType == 2) { // Sepia
          monotoneColor[0] = 1.0f;
          monotoneColor[1] = 74.0f / 107.0f;
          monotoneColor[2] = 43.0f / 107.0f;
      }
  }

  bool useGrayscale = (monotoneType != 0);
  
  if (monotoneType == 3) {
      ImGui::ColorEdit3("Custom Monotone Color", monotoneColor);
  }

  if (postProcess_) {
    postProcess_->SetUseGrayscale(useGrayscale);
    postProcess_->SetMonotoneColor(monotoneColor[0], monotoneColor[1], monotoneColor[2]);
  }
  ImGui::End();

  // ImGui受付終了
  if (imGuiManager_) {
    imGuiManager_->End();
  }

#endif // USE_IMGUI
}

void Game::Draw() {

  EngineBase::BeginFrame();

  // 3D
  // EngineBase::Begin3D();
  SceneManager::GetInstance()->Draw();

  // Swapchainに描画先を切り替える
  dx12Core_->PreDrawImGui();

  // PostProcessでRenderTextureをSwapchainに描画する
  postProcess_->Draw(renderTextureSrvIndex_, srvManager_.get());

  // 2D
  // EngineBase::Begin2D();

  if (imGuiManager_) {
    imGuiManager_->Draw();
  }

  EngineBase::EndFrame();
}
