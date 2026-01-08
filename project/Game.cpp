#include "Game.h"
#include "Camera/DebugCamera.h"
#include "Camera/GameCamera.h"
#include "Core/D3DResourceLeakChecker.h"
#include "Core/SrvManager.h"
#include "Core/WindowSystem.h"
#include "Input/InputKeyState.h"
#include "Render/Model.h"
#include "Render/ModelManager.h"
#include "Render/ModelRenderer.h"
#include "Render/Object3d.h"
#include "Render/Object3dRenderer.h"
#include "Render/Particle.h"
#include "Render/ParticleEmitter.h"
#include "Render/ParticleManager.h"
#include "Render/ResourceObject.h"
#include "Render/Sprite.h"
#include "Render/SpriteRenderer.h"
#include "Render/TextureManager.h"
#include <cassert>
#include <fstream>

// チャンクヘッダ
struct ChunkHeader {
  char id[4];   // チャンク毎のID
  int32_t size; // チャンクサイズ
};

// RIFFヘッダチャンク
struct RiffHeader {
  ChunkHeader chunk; //"RIFF"
  char type[4];      //"WAVE"
};

// FMTチャンク
struct FormatChunk {
  ChunkHeader chunk; //"fmt"
  WAVEFORMATEX fmt;  // 波形フォーマット
};

/// <summary>
/// waveファイルを読み込む
/// </summary>
/// <param name="filename">ファイル名</param>
/// <returns></returns>
SoundData SoundLoadWave(const char *filename) {

  /* ファイルオープン
  ----------------------*/

  // ファイル入力ストリームのインスタンス
  std::ifstream file;

  //.wavファイルをバイナリモードで開く
  file.open(filename, std::ios_base::binary);

  // ファイルが開けなかったら
  assert(file.is_open());

  /* .wavデータ読み込み
  ----------------------*/

  // RIFFヘッダーの読み込み
  RiffHeader riff;

  file.read((char *)&riff, sizeof(riff));

  // 開いたファイルがRIFFであるかを確認する
  if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
    assert(0);
  }

  // タイプがWAVEか確認
  if (strncmp(riff.type, "WAVE", 4) != 0) {
    assert(0);
  }

  // formatチャンクの読み込み
  FormatChunk format = {};

  // チャンクヘッダーの確認
  file.read((char *)&format, sizeof(ChunkHeader));

  if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
    assert(0);
  }

  // チャンク本体の読み込み
  assert(format.chunk.size <= sizeof(format.fmt));
  file.read((char *)&format.fmt, format.chunk.size);

  // Dataチャンクの読み込み
  ChunkHeader data;

  file.read((char *)&data, sizeof(data));

  // Junkチャンク(パディング？)を検出した場合
  if (strncmp(data.id, "JUNK", 4) == 0) {

    // 読み取り位置をJunkチャンクの終わりまで進める
    file.seekg(data.size, std::ios_base::cur);

    // 飛ばした後に再度読み込み
    file.read((char *)&data, sizeof(data));
  }

  if (strncmp(data.id, "data", 4) != 0) {
    assert(0);
  }

  // Dataチャンクのデータ部(波形データ)の読み込み
  char *pBuffer = new char[data.size];
  file.read(pBuffer, data.size);

  // Waveファイルを閉じる
  file.close();

  /* 読み込んだ音声データをreturn
  -------------------------------*/

  // returnするための音声データ
  SoundData soundData = {};

  soundData.wfex = format.fmt;
  soundData.pBuffer = reinterpret_cast<BYTE *>(pBuffer);
  soundData.bufferSize = data.size;

  return soundData;
}

/// <summary>
/// 音声データの解放
/// </summary>
/// <param name="soundData"></param>
void SoundUnload(SoundData *soundData) {

  // バッファのメモリを解放
  delete[] soundData->pBuffer;

  soundData->pBuffer = 0;
  soundData->bufferSize = 0;
  soundData->wfex = {};
}

/// <summary>
/// サウンド再生
/// </summary>
/// <param name="xAudio2">再生するためのxAudio2</param>
/// <param name="soundData">音声データ(波形データ、サイズ、フォーマット)</param>
void SoundPlayerWave(IXAudio2 *xAudio2, const SoundData &soundData) {

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

  //===========================
  // ローカル変数宣言
  //===========================

  // hrの生成
  HRESULT hr;

#ifdef _DEBUG
  leakChecker_ = new D3DResourceLeakChecker();
#endif

  MSG msg{};

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
  hr = xAudio2_->CreateMasteringVoice(&masterVoice);
  assert(SUCCEEDED(hr));

  //===========================
  // SRVManagerの初期化
  //===========================
  srvManager_ = new SrvManager();
  srvManager_->Initialize(dx12Core_);

  //===========================
  // テクスチャファイルの読み込み
  //===========================

  // テクスチャマネージャーの初期化
  TextureManager::GetInstance()->Initialize(dx12Core_, srvManager_);

  // テクスチャの読み込み
  TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

  TextureManager::GetInstance()->LoadTexture("resources/monsterball.png");

  //===========================
  // スプライト関係の初期化
  //===========================

  spriteRenderer_ = new SpriteRenderer();
  spriteRenderer_->Initialize(dx12Core_);

  // スプライトの生成と初期化
  for (int i = 0; i < kSpriteCount_; ++i) {
    Sprite *sprite = new Sprite();
    if (i % 2 == 1) {
      sprite->Initialize(spriteRenderer_, "resources/uvChecker.png");
    } else {
      sprite->Initialize(spriteRenderer_, "resources/monsterball.png");
    }
    sprites_.push_back(sprite);
  }

  sprite_ = new Sprite();
  sprite_->Initialize(spriteRenderer_, "resources/uvChecker.png");

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    spritePositions_[i] = {i * 200.0f, 0.0f};
    spriteSizes_[i] = {150.0f, 150.0f};
  }

  //===========================
  // デバッグテキストの初期化(未実装)
  //===========================

  //===========================
  // 3Dオブジェクト関係の初期化
  //===========================
  object3dRenderer_ = new Object3dRenderer();
  object3dRenderer_->Initialize(dx12Core_);

  // モデルマネージャーの初期化
  ModelManager::GetInstance()->Initialize(dx12Core_);

  // モデルの読み込み
  ModelManager::GetInstance()->LoadModel("plane.obj");

  ModelManager::GetInstance()->LoadModel("axis.obj");

  // カメラの生成と初期化
  camera_ = new GameCamera();
  camera_->SetRotate({0.3f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 4.0f, -10.0f});
  object3dRenderer_->SetDefaultCamera(camera_);

  // オブジェクトの生成と初期化
  object3d_ = new Object3d();
  object3d_->Initialize(object3dRenderer_);
  object3d_->SetModel("plane.obj");

  object3dA_ = new Object3d();
  object3dA_->Initialize(object3dRenderer_);
  object3dA_->SetModel("axis.obj");

  //===========================
  // オーディオファイルの読み込み
  //===========================
  soundData1_ = SoundLoadWave("resources/fanfare.wav");

  //===========================
  // パーティクル関係の初期化
  //===========================
  ParticleManager::GetInstance()->Initialize(dx12Core_, srvManager_);

  // グループ登録（name と texture を紐づけ）
  ParticleManager::GetInstance()->CreateParticleGroup("test",
                                                      "resources/circle.png");

  // エミッタ
  Transform emitterTransform{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 5.0f},
  };

  particleEmitter_ =
      new ParticleEmitter("test", emitterTransform, 3, 1.0f, 0.0f);

  // UVTransfotm用
  uvTransformSprite_ = {
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  // texture切り替え用
  bool useMonsterBall = true;

  // Lighting
  bool enableLighting = true;

  // デバッグカメラ
  debugCamera_ = new DebugCamera();
  debugCamera_->Initialize();

  // デルタタイム 一旦60fpsで固定
  const float kDeltaTime = 1.0f / 60.0f;

  bool isUpdate = false;

  bool useBillboard = true;

  // スプライトのTransform
  spritePosition_ = {
      640.0f,
      360.0f,
  };

  spriteRotation_ = 0.0f;

  spriteColor_ = {1.0f, 1.0f, 1.0f, 1.0f};

  spriteSize_ = {640.0f, 360.0f};

  spriteAnchorPoint_ = {0.5f, 0.5f};

  sprite_->SetPosition(spritePosition_);
  sprite_->SetRotation(spriteRotation_);
  sprite_->SetColor(spriteColor_);
  sprite_->SetSize(spriteSize_);
  sprite_->SetAnchorPoint(spriteAnchorPoint_);
  sprite_->SetIsFlipX(isFlipX_);
  sprite_->SetIsFlipY(isFlipY_);

  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f},
      {0.3f, 0.0f, 0.0f},
      {0.0f, 4.0f, -10.0f},
  };

  //// 風を出す範囲 ※今は使用していないのでコメントアウト
  // AccelerationField accelerationField;
  // accelerationField.acceleration = {15.0f, 0.0f, 0.0f};
  // accelerationField.area.min = {-1.0f, -1.0f, -1.0f};
  // accelerationField.area.max = {1.0f, 1.0f, 1.0f};
}

void Game::Finalize() {

  xAudio2_.Reset();

  delete debugCamera_;

  delete particleEmitter_;
  particleEmitter_ = nullptr;

  ModelManager::GetInstance()->Finalize();

  TextureManager::GetInstance()->Finalize();

  ParticleManager::GetInstance()->Finalize();

  delete input_;

  delete object3dA_;

  delete object3d_;

  delete sprite_;

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    delete sprites_[i];
  }

  delete srvManager_;
  srvManager_ = nullptr;

  delete camera_;
  camera_ = nullptr;

  delete object3dRenderer_;

  delete spriteRenderer_;

  delete dx12Core_;

  windowSystem_->Finalize();

  delete windowSystem_;
  windowSystem_ = nullptr;

  SoundUnload(&soundData1_);

#ifdef _DEBUG
  delete leakChecker_;
  leakChecker_ = nullptr;
#endif
}

void Game::Update() {

  // Windowにメッセージが来てたら最優先で処理させる
  if (windowSystem_ && windowSystem_->ProcessMessage()) {
    endRequest_ = true;
    return;
  }

  // キー入力
  input_->Update();

  //=======================
  // アクターの更新
  //=======================

  // エミッタ更新
  particleEmitter_->Update();

  // view / projection を作って ParticleManager 更新
  Matrix4x4 cameraMatrix =
      MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate,
                       cameraTransform_.translate);

  Matrix4x4 viewMatrix = Inverse(cameraMatrix);

  Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
      0.45f,
      float(WindowSystem::kClientWidth) / float(WindowSystem::kClientHeight),
      0.1f, 100.0f);

  ParticleManager::GetInstance()->Update(viewMatrix, projectionMatrix);

  debugCamera_->Update(*input_);

  // FPSをセット
  dx12Core_->SetFPS(set60FPS_);

  // テクスチャ差し替え
  if (input_->IsTriggerKey(DIK_SPACE)) {
    sprites_[0]->ChangeTexture("resources/uvChecker.png");
  }

  camera_->SetRotate(cameraTransform_.rotate);
  camera_->SetTranslate(cameraTransform_.translate);

  // カメラの更新処理
  camera_->Update();

  //=======================
  // スプライトの更新
  //=======================

  // スプライトの情報を呼び出す
  spritePosition_ = sprite_->GetPosition();
  spriteRotation_ = sprite_->GetRotation();
  spriteColor_ = sprite_->GetColor();
  spriteSize_ = sprite_->GetSize();
  spriteAnchorPoint_ = sprite_->GetAnchorPoint();

  // スプライトに設定
  sprite_->SetPosition(spritePosition_);
  sprite_->SetRotation(spriteRotation_);
  sprite_->SetColor(spriteColor_);
  sprite_->SetSize(spriteSize_);
  sprite_->SetAnchorPoint(spriteAnchorPoint_);
  sprite_->SetIsFlipX(isFlipX_);
  sprite_->SetIsFlipY(isFlipY_);

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    // spritePositions[i].x++;
    sprites_[i]->SetPosition(spritePositions_[i]);
    sprites_[i]->SetSize(spriteSizes_[i]);
  }

  sprite_->Update(uvTransformSprite_);

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    sprites_[i]->Update(uvTransformSprite_);
  }

  //=======================
  // デバッグテキストの更新
  //=======================

  //=======================
  // 3Dオブジェクトの更新
  //=======================
  rotateObj_ += 0.01f;

  object3d_->SetRotation({0.0f, rotateObj_, 0.0f});

  object3dA_->SetRotation({0.0f, rotateObj_, 0.0f});
  object3dA_->SetTranslation({1.0f, 1.0f, 0.0f});

  object3d_->Update();
  object3dA_->Update();
}

void Game::Draw() {

  // DirectXの描画準備。すべてに共通のグラフィックスコマンドを積む
  dx12Core_->BeginFrame();

  // SRVをコマンドリストにセット
  srvManager_->PreDraw();

  // 3DObjectの描画準備。3DObjectの描画に共通のグラフィックスコマンドを積む
  object3dRenderer_->Begin();

  // ３Dオブジェクトの描画
  object3d_->Draw();
  object3dA_->Draw();

  ParticleManager::GetInstance()->Draw();

  // Spriteの描画準備。Spriteの描画に共通のグラフィックスコマンドを積む
  spriteRenderer_->Begin();

  // スプライトの描画
  // sprite->Draw();

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    sprites_[i]->Draw();
  }

  dx12Core_->EndFrame();
}
