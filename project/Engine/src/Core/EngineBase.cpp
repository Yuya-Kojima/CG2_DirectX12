#include "Core/EngineBase.h"
#include "Camera/GameCamera.h"
#include "Core/D3DResourceLeakChecker.h"
#include "Core/SrvManager.h"
#include "Core/WindowSystem.h"
#include "Input/InputKeyState.h"
#include "Model/ModelManager.h"
#include "Particle/ParticleManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Texture/TextureManager.h"
#include <cassert>

void EngineBase::Initialize() {

  // hrの生成
  HRESULT hr;

#ifdef _DEBUG
  leakChecker_ = new D3DResourceLeakChecker();
#endif

  //===========================
  // WindowsAPIの初期化
  //===========================
  windowSystem_ = new WindowSystem();
  windowSystem_->Initialize();

  //===========================
  // DirectXの初期化
  //===========================
  dx12Core_ = new Dx12Core();
  dx12Core_->Initialize(windowSystem_);

  ID3D12Device *device = dx12Core_->GetDevice();
  ID3D12GraphicsCommandList *commandList = dx12Core_->GetCommandList();

  //===========================
  // キーボード入力の初期化
  //===========================
  input_ = new InputKeyState();
  input_->Initialize(windowSystem_);

  //===========================
  // Audioの初期化
  //===========================
  IXAudio2MasteringVoice
      *masterVoice; // xAudio2が解放されると同時に無効化されるのでdeleteしない。

  hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
  assert(SUCCEEDED(hr));
  hr = xAudio2_->CreateMasteringVoice(&masterVoice);
  assert(SUCCEEDED(hr));

  //===========================
  // SRVManagerの初期化
  //===========================
  srvManager_ = new SrvManager();
  srvManager_->Initialize(dx12Core_);

  spriteRenderer_ = new SpriteRenderer();
  spriteRenderer_->Initialize(dx12Core_);

  object3dRenderer_ = new Object3dRenderer();
  object3dRenderer_->Initialize(dx12Core_);

  // テクスチャマネージャーの初期化
  TextureManager::GetInstance()->Initialize(dx12Core_, srvManager_);

  // モデルマネージャーの初期化
  ModelManager::GetInstance()->Initialize(dx12Core_);

  // パーティクルマネージャーの初期化
  ParticleManager::GetInstance()->Initialize(dx12Core_, srvManager_);
}

void EngineBase::Finalize() {
  xAudio2_.Reset();

  ParticleManager::GetInstance()->Finalize();

  ModelManager::GetInstance()->Finalize();

  TextureManager::GetInstance()->Finalize();

  delete input_;
  input_ = nullptr;

  delete srvManager_;
  srvManager_ = nullptr;

  delete object3dRenderer_;
  object3dRenderer_ = nullptr;

  delete spriteRenderer_;
  spriteRenderer_ = nullptr;

  delete dx12Core_;
  dx12Core_ = nullptr;

  windowSystem_->Finalize();

  delete windowSystem_;
  windowSystem_ = nullptr;

#ifdef _DEBUG
  delete leakChecker_;
  leakChecker_ = nullptr;
#endif
}

void EngineBase::Update() {

  // Windowにメッセージが来てたら最優先で処理させる
  if (windowSystem_ && windowSystem_->ProcessMessage()) {
    endRequest_ = true;
    return;
  }

  // キー入力
  input_->Update();
}

void EngineBase::BeginFrame() {
  // DirectXの描画準備。すべてに共通のグラフィックスコマンドを積む
  dx12Core_->BeginFrame();

  // SRVをコマンドリストにセット
  srvManager_->PreDraw();
}

void EngineBase::EndFrame() { dx12Core_->EndFrame(); }

void EngineBase::Begin3D() {
  // 3DObjectの描画準備。3DObjectの描画に共通のグラフィックスコマンドを積む
  object3dRenderer_->Begin();
}

void EngineBase::Begin2D() {
  // Spriteの描画準備。Spriteの描画に共通のグラフィックスコマンドを積む
  spriteRenderer_->Begin();
}

void EngineBase::End3D() {
  // 今は何もしない
}

void EngineBase::End2D() {
  // 今は何もしない
}

void EngineBase::Run() {
  Initialize();

  while (true) {
    Update();
    if (IsEndRequest()) {
      break;
    }
    Draw();
  }

  Finalize();
}
