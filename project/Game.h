#pragma once
#include "Math/MathUtil.h"
#include <vector>
#include <wrl.h>
#include <xaudio2.h>

class WindowSystem;
class InputKeyState;
class SpriteRenderer;
class Sprite;
class Object3dRenderer;
class GameCamera;
class SrvManager;
class Object3d;
class ParticleEmitter;
class DebugCamera;
class Dx12Core;
class D3DResourceLeakChecker;

// 音声データ
struct SoundData {
  // 波形フォーマット
  WAVEFORMATEX wfex;

  // バッファの先頭アドレス
  BYTE *pBuffer;

  // バッファサイズ
  unsigned int bufferSize;
};

class Game {

public:
  void Initialize();

  void Finalize();

  void Update();

  void Draw();

private:
  // ゲーム終了フラグ
  bool endRequest_ = false;

public:
  bool IsEndRequest() { return endRequest_; }

private:
  D3DResourceLeakChecker *leakChecker_ = nullptr;

  WindowSystem *windowSystem_ = nullptr;

  Dx12Core *dx12Core_ = nullptr;

  InputKeyState *input_ = nullptr;

  Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;

  SoundData soundData1_;

  SpriteRenderer *spriteRenderer_ = nullptr;

  Sprite *sprite_ = nullptr;

  static constexpr int kSpriteCount_ = 5;

  std::vector<Sprite *> sprites_;

  Vector2 spritePositions_[kSpriteCount_];

  Vector2 spriteSizes_[kSpriteCount_];

  Object3dRenderer *object3dRenderer_ = nullptr;

  GameCamera *camera_ = nullptr;

  Object3d *object3d_ = nullptr;
  Object3d *object3dA_ = nullptr;

  SrvManager *srvManager_ = nullptr;

  ParticleEmitter *particleEmitter_ = nullptr;

  Transform cameraTransform_;

  DebugCamera *debugCamera_ = nullptr;

  bool set60FPS_ = false;

  // スプライトのTransform
  Vector2 spritePosition_;

  float spriteRotation_;

  Vector4 spriteColor_;

  Vector2 spriteSize_;

  Vector2 spriteAnchorPoint_;

  bool isFlipX_ = false;
  bool isFlipY_ = false;

  Transform uvTransformSprite_;

  float rotateObj_;
};
