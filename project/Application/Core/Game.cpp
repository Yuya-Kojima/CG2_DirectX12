#include "Core/Game.h"
#include "Core/ResourceObject.h"
#include "Core/SrvManager.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Renderer/ModelRenderer.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/GamePlayScene.h"
#include "Scene/TitleScene.h"
#include "Sprite/Sprite.h"
#include "Texture/TextureManager.h"
#include <cassert>
#include <fstream>

/// <summary>
/// サウンド再生
/// </summary>
/// <param name="xAudio2">再生するためのxAudio2</param>
/// <param name="soundData">音声データ(波形データ、サイズ、フォーマット)</param>
void SoundPlayWave(IXAudio2 *xAudio2, const SoundData &soundData) {

  HRESULT result;

  // 波形フォーマットを元にSourceVoiceの生成
  IXAudio2SourceVoice *pSourceVoice = nullptr;
  result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
  assert(SUCCEEDED(result));

  // 再生する波形データの設定
  XAUDIO2_BUFFER buf{};
  buf.pAudioData = soundData.pBuffer;
  buf.AudioBytes = soundData.bufferSize;
  buf.Flags = XAUDIO2_END_OF_STREAM;

  // 波形データの再生
  result = pSourceVoice->SubmitSourceBuffer(&buf);
  result = pSourceVoice->Start();
}

void Game::Initialize() {

  // 基底クラスの初期化処理
  EngineBase::Initialize();

  // シーンの初期化
  scene_ = new GamePlayScene();

  scene_->Initialize(this);

  //===========================
  // ローカル変数宣言
  //===========================

  //===========================
  // デバッグテキストの初期化(未実装)
  //===========================

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

  scene_->Finalize();
  delete scene_;

  // 基底クラスの終了処理
  EngineBase::Finalize();
}

void Game::Update() {

  // 基底クラスの更新処理
  EngineBase::Update();
  if (IsEndRequest()) {
    return;
  }

  scene_->Update();

  // FPSをセット
  dx12Core_->SetFPS(set60FPS_);

  //=======================
  // アクターの更新
  //=======================

  //=======================
  // デバッグテキストの更新
  //=======================
}

void Game::Draw() {

  EngineBase::BeginFrame();

  // 3D
  EngineBase::Begin3D();
  scene_->Draw3D();

  // 2D
  EngineBase::Begin2D();
  scene_->Draw2D();

  EngineBase::EndFrame();
}
