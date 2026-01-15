#pragma once
#include <wrl.h>
#include <xaudio2.h>

class WindowSystem;
class InputKeyState;
class Dx12Core;
class Object3dRenderer;
class SpriteRenderer;
class SrvManager;
class GameCamera;
class D3DResourceLeakChecker;
class AbstractSceneFactory;

// 音声データ
struct SoundData {
  // 波形フォーマット
  WAVEFORMATEX wfex;

  // バッファの先頭アドレス
  BYTE *pBuffer;

  // バッファサイズ
  unsigned int bufferSize;
};

class EngineBase {
public:
  // 仮想デストラクタ
  virtual ~EngineBase() = default;

  // 初期化
  virtual void Initialize();

  // 終了
  virtual void Finalize();

  // 毎フレーム更新
  virtual void Update();

  // 描画
  virtual void Draw() = 0;

  void BeginFrame();
  void EndFrame();
  void Begin3D();
  void Begin2D();
  void End3D();
  void End2D();

  // 終了チェック
  virtual bool IsEndRequest() const { return endRequest_; }

  // 実行
  void Run();

  InputKeyState *GetInputManager() const { return input_; }
  SpriteRenderer *GetSpriteRenderer() const { return spriteRenderer_; }
  Object3dRenderer *GetObject3dRenderer() const { return object3dRenderer_; }

protected:
  bool endRequest_ = false;

protected:
  D3DResourceLeakChecker *leakChecker_ = nullptr;

  WindowSystem *windowSystem_ = nullptr;

  Dx12Core *dx12Core_ = nullptr;

  InputKeyState *input_ = nullptr;

  Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;

  SpriteRenderer *spriteRenderer_ = nullptr;

  Object3dRenderer *object3dRenderer_ = nullptr;

  SrvManager *srvManager_ = nullptr;

  // シーンファクトリー
  AbstractSceneFactory *sceneFactory_ = nullptr;
};
