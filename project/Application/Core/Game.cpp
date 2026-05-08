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

  if (ImGui::CollapsingHeader("PostEffect", ImGuiTreeNodeFlags_DefaultOpen)) {
      
      // --- Base Effect ---
      ImGui::Separator();
      ImGui::Text("Base Effect");
      static int postEffectType = 0;
      const char* effectTypes[] = { "None", "BoxFilter" };
      ImGui::Combo("Effect Type", &postEffectType, effectTypes, IM_ARRAYSIZE(effectTypes));
      
      if (postEffectType == 1) { // BoxFilter
          static int boxFilterK = 1;
          ImGui::DragInt("BoxFilter K (Radius)", &boxFilterK, 0.1f, 1, 10);
          if (postProcess_) postProcess_->SetBoxFilterK(boxFilterK);
      }
      if (postProcess_) {
          postProcess_->SetPostEffectType(postEffectType);
      }

      // --- Monotone ---
      ImGui::Separator();
      ImGui::Text("Color Grading");
      static int monotoneType = 0; // 0: Normal, 1: Grayscale, 2: Sepia, 3: Custom
      const char* monotoneItems[] = { "Normal (Off)", "Grayscale", "Sepia", "Custom" };
      ImGui::Combo("Monotone", &monotoneType, monotoneItems, IM_ARRAYSIZE(monotoneItems));
      
      static float monotoneColor[3] = {1.0f, 1.0f, 1.0f};
      if (monotoneType == 1) {
          monotoneColor[0] = 1.0f; monotoneColor[1] = 1.0f; monotoneColor[2] = 1.0f;
      } else if (monotoneType == 2) {
          monotoneColor[0] = 1.0f; monotoneColor[1] = 74.0f / 107.0f; monotoneColor[2] = 43.0f / 107.0f;
      } else if (monotoneType == 3) {
          ImGui::ColorEdit3("Custom Color", monotoneColor);
      }
      
      if (postProcess_) {
          postProcess_->SetUseGrayscale(monotoneType != 0);
          postProcess_->SetMonotoneColor(monotoneColor[0], monotoneColor[1], monotoneColor[2]);
      }

      // --- Vignette ---
      ImGui::Separator();
      ImGui::Text("Vignette");
      static int vignetteType = 0;
      const char* vignetteItems[] = { "Normal (Off)", "Mild", "Strong", "Pinhole", "Custom" };
      ImGui::Combo("Vignette", &vignetteType, vignetteItems, IM_ARRAYSIZE(vignetteItems));
      
      static float vignetteScale = 16.0f;
      static float vignetteExponent = 0.8f;
      if (vignetteType == 1) {
          vignetteScale = 16.0f; vignetteExponent = 0.8f;
      } else if (vignetteType == 2) {
          vignetteScale = 16.0f; vignetteExponent = 1.5f;
      } else if (vignetteType == 3) {
          vignetteScale = 5.0f; vignetteExponent = 1.2f;
      } else if (vignetteType == 4) {
          ImGui::DragFloat("Scale", &vignetteScale, 0.1f, 1.0f, 100.0f);
          ImGui::DragFloat("Exponent", &vignetteExponent, 0.05f, 0.1f, 5.0f);
      }
      
      if (postProcess_) {
          postProcess_->SetUseVignette(vignetteType != 0);
          postProcess_->SetVignetteScale(vignetteScale);
          postProcess_->SetVignetteExponent(vignetteExponent);
      }
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
