#include "TitleScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Input/InputKeyState.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Sprite/Sprite.h"
#include "Texture/TextureManager.h"
#include <fstream>

//// チャンクヘッダ
//struct ChunkHeader {
//  char id[4];   // チャンク毎のID
//  int32_t size; // チャンクサイズ
//};
//
//// RIFFヘッダチャンク
//struct RiffHeader {
//  ChunkHeader chunk; //"RIFF"
//  char type[4];      //"WAVE"
//};
//
//// FMTチャンク
//struct FormatChunk {
//  ChunkHeader chunk; //"fmt"
//  WAVEFORMATEX fmt;  // 波形フォーマット
//};
//
///// <summary>
///// waveファイルを読み込む
///// </summary>
///// <param name="filename">ファイル名</param>
///// <returns></returns>
//SoundData SoundLoadWave(const char *filename) {
//
//  /* ファイルオープン
//  ----------------------*/
//
//  // ファイル入力ストリームのインスタンス
//  std::ifstream file;
//
//  //.wavファイルをバイナリモードで開く
//  file.open(filename, std::ios_base::binary);
//
//  // ファイルが開けなかったら
//  assert(file.is_open());
//
//  /* .wavデータ読み込み
//  ----------------------*/
//
//  // RIFFヘッダーの読み込み
//  RiffHeader riff;
//
//  file.read((char *)&riff, sizeof(riff));
//
//  // 開いたファイルがRIFFであるかを確認する
//  if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
//    assert(0);
//  }
//
//  // タイプがWAVEか確認
//  if (strncmp(riff.type, "WAVE", 4) != 0) {
//    assert(0);
//  }
//
//  // formatチャンクの読み込み
//  FormatChunk format = {};
//
//  // チャンクヘッダーの確認
//  file.read((char *)&format, sizeof(ChunkHeader));
//
//  if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
//    assert(0);
//  }
//
//  // チャンク本体の読み込み
//  assert(format.chunk.size <= sizeof(format.fmt));
//  file.read((char *)&format.fmt, format.chunk.size);
//
//  // Dataチャンクの読み込み
//  ChunkHeader data;
//
//  file.read((char *)&data, sizeof(data));
//
//  // Junkチャンク(パディング？)を検出した場合
//  if (strncmp(data.id, "JUNK", 4) == 0) {
//
//    // 読み取り位置をJunkチャンクの終わりまで進める
//    file.seekg(data.size, std::ios_base::cur);
//
//    // 飛ばした後に再度読み込み
//    file.read((char *)&data, sizeof(data));
//  }
//
//  if (strncmp(data.id, "data", 4) != 0) {
//    assert(0);
//  }
//
//  // Dataチャンクのデータ部(波形データ)の読み込み
//  char *pBuffer = new char[data.size];
//  file.read(pBuffer, data.size);
//
//  // Waveファイルを閉じる
//  file.close();
//
//  /* 読み込んだ音声データをreturn
//  -------------------------------*/
//
//  // returnするための音声データ
//  SoundData soundData = {};
//
//  soundData.wfex = format.fmt;
//  soundData.pBuffer = reinterpret_cast<BYTE *>(pBuffer);
//  soundData.bufferSize = data.size;
//
//  return soundData;
//}
//
///// <summary>
///// 音声データの解放
///// </summary>
///// <param name="soundData"></param>
//void SoundUnload(SoundData *soundData) {
//
//  // バッファのメモリを解放
//  delete[] soundData->pBuffer;
//
//  soundData->pBuffer = 0;
//  soundData->bufferSize = 0;
//  soundData->wfex = {};
//}

void TitleScene::Initialize(EngineBase *engine) {

  engine_ = engine;

  //===========================
  // テクスチャファイルの読み込み
  //===========================

  // テクスチャの読み込み
  TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

  TextureManager::GetInstance()->LoadTexture("resources/monsterball.png");

  //===========================
  // オーディオファイルの読み込み
  //===========================
  //soundData1_ = SoundLoadWave("resources/fanfare.wav");

  //===========================
  // スプライト関係の初期化
  //===========================

  // スプライトの生成と初期化
  for (int i = 0; i < kSpriteCount_; ++i) {
    Sprite *sprite = new Sprite();
    if (i % 2 == 1) {
      sprite->Initialize(engine_->GetSpriteRenderer(),
                         "resources/uvChecker.png");
    } else {
      sprite->Initialize(engine_->GetSpriteRenderer(),
                         "resources/uvChecker.png");
    }
    sprites_.push_back(sprite);
  }

  sprite_ = new Sprite();
  sprite_->Initialize(engine_->GetSpriteRenderer(), "resources/uvChecker.png");

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    spritePositions_[i] = {i * 200.0f, 0.0f};
    spriteSizes_[i] = {150.0f, 150.0f};
  }

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

  // UVTransform用
  uvTransformSprite_ = {
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  //===========================
  // 3Dオブジェクト関係の初期化
  //===========================

  // カメラの生成と初期化
  camera_ = new GameCamera();
  camera_->SetRotate({0.3f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 4.0f, -10.0f});
  engine_->GetObject3dRenderer()->SetDefaultCamera(camera_);

  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f},
      {0.3f, 0.0f, 0.0f},
      {0.0f, 4.0f, -10.0f},
  };

  // デバッグカメラ
  debugCamera_ = new DebugCamera();
  debugCamera_->Initialize();

  // モデルの読み込み
  ModelManager::GetInstance()->LoadModel("plane.obj");

  ModelManager::GetInstance()->LoadModel("axis.obj");

  // オブジェクトの生成と初期化
  object3d_ = new Object3d();
  object3d_->Initialize(engine_->GetObject3dRenderer());
  object3d_->SetModel("plane.obj");

  object3dA_ = new Object3d();
  object3dA_->Initialize(engine_->GetObject3dRenderer());
  object3dA_->SetModel("axis.obj");

  //===========================
  // パーティクル関係の初期化
  //===========================

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
}

void TitleScene::Finalize() {
  delete object3dA_;
  object3dA_ = nullptr;

  delete object3d_;
  object3d_ = nullptr;

  delete debugCamera_;
  debugCamera_ = nullptr;

  delete camera_;
  camera_ = nullptr;

  delete sprite_;
  sprite_ = nullptr;

  delete particleEmitter_;
  particleEmitter_ = nullptr;

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    delete sprites_[i];
  }
  sprites_.clear();

  //SoundUnload(&soundData1_);
}

void TitleScene::Update() {

  // テクスチャ差し替え
  if (engine_->GetInputManager()->IsTriggerKey(DIK_SPACE)) {
    sprites_[0]->ChangeTexture("resources/uvChecker.png");
  }

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
  // 3Dオブジェクトの更新
  //=======================
  rotateObj_ += 0.01f;

  object3d_->SetRotation({0.0f, rotateObj_, 0.0f});

  object3dA_->SetRotation({0.0f, rotateObj_, 0.0f});
  object3dA_->SetTranslation({1.0f, 1.0f, 0.0f});

  object3d_->Update();
  object3dA_->Update();

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

  debugCamera_->Update(*engine_->GetInputManager());

  camera_->SetRotate(cameraTransform_.rotate);
  camera_->SetTranslate(cameraTransform_.translate);

  // カメラの更新処理
  camera_->Update();
}

void TitleScene::Draw3D() {
  object3d_->Draw();
  object3dA_->Draw();
  ParticleManager::GetInstance()->Draw();
}

void TitleScene::Draw2D() {
  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    sprites_[i]->Draw();
  }
}