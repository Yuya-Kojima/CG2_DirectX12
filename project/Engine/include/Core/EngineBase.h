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
class ImGuiManager;

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
  // 解放漏れリークチェッカー
  D3DResourceLeakChecker *leakChecker_ = nullptr;

  // WinApp
  WindowSystem *windowSystem_ = nullptr;

  // DirectXCommon
  Dx12Core *dx12Core_ = nullptr;

  // 入力
  InputKeyState *input_ = nullptr;

  // サウンド
  Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;

  // 2dスプライト
  SpriteRenderer *spriteRenderer_ = nullptr;

  // 3Ⅾオブジェクト
  Object3dRenderer *object3dRenderer_ = nullptr;

  // SRV
  SrvManager *srvManager_ = nullptr;

  // シーンファクトリー
  AbstractSceneFactory *sceneFactory_ = nullptr;

  // ImGui
  ImGuiManager *imGuiManager_ = nullptr;
};
