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

  // RenderTextureの作成
  const Vector4 kRenderTargetClearValue{ 0.0f, 0.0f, 0.0f, 1.0f }; // 背景は黒
  mainRenderTexture_ = std::make_unique<RenderTexture>();
  mainRenderTexture_->Initialize(dx12Core_.get(), srvManager_.get(), WindowSystem::kClientWidth, WindowSystem::kClientHeight, kRenderTargetClearValue, DXGI_FORMAT_R16G16B16A16_FLOAT);

  // Bloomの初期化
  bloom_ = std::make_unique<Bloom>();
  bloom_->Initialize(dx12Core_.get(), srvManager_.get(), WindowSystem::kClientWidth, WindowSystem::kClientHeight);

  // DepthTextureのSRV作成
  depthTextureSrvIndex_ = srvManager_->Allocate();
  srvManager_->CreateSRVforDepth(depthTextureSrvIndex_,
                                 dx12Core_->GetDepthStencilResource());

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

  // メインのオフスクリーンテクスチャを描画対象に設定
  mainRenderTexture_->TransitionToRenderTarget(dx12Core_.get());
  auto commandList = dx12Core_->GetCommandList();
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mainRenderTexture_->GetRtvHandle();
  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dx12Core_->GetDsvCpuDescriptorHandle();
  commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
  
  // 画面クリア
  mainRenderTexture_->Clear(dx12Core_.get());

  // 3D描画
  SceneManager::GetInstance()->Draw();

  // ----- ブルーム処理の実行 -----
  // mainRenderTexture_ を ShaderResource にする（抽出パスで読むため）
  mainRenderTexture_->TransitionToShaderResource(dx12Core_.get());
  uint32_t bloomSrvIndex = 0;

  if (auto pp = SceneManager::GetInstance()->GetCurrentScenePostProcess()) {
    // ImGuiの設定をBloomに反映
    bloom_->SetThreshold(pp->GetBloomThreshold());
    bloom_->SetSigma(pp->GetBloomSigma());
    
    // ブルームがONの時だけ描画を実行
    if (pp->GetUseBloom()) {
      bloomSrvIndex = bloom_->Draw(dx12Core_.get(), srvManager_.get(), mainRenderTexture_->GetSrvIndex());
    }
  } else {
    bloomSrvIndex = bloom_->Draw(dx12Core_.get(), srvManager_.get(), mainRenderTexture_->GetSrvIndex());
  }

  // Swapchainに描画先を切り替える
  dx12Core_->PreDrawImGui();

  // プロジェクション逆行列の取得とセット
  if (auto pp = SceneManager::GetInstance()->GetCurrentScenePostProcess()) {
    const ICamera *defaultCamera = object3dRenderer_->GetDefaultCamera();
    if (defaultCamera) {
      pp->SetProjectionInverse(Inverse(defaultCamera->GetProjectionMatrix()));
    }
    
    // ブルーム結果をセット
    pp->SetBloomSrvIndex(bloomSrvIndex);
    
    pp->Draw(mainRenderTexture_->GetSrvIndex(), depthTextureSrvIndex_, srvManager_.get());
  }

  // 2Dオーバーレイ描画パス（ポストプロセスの後に上書き描画する）
  SceneManager::GetInstance()->DrawOverlay();

  // 2D
  // EngineBase::Begin2D();

  if (imGuiManager_) {
    imGuiManager_->Draw();
  }

  EngineBase::EndFrame();
}
