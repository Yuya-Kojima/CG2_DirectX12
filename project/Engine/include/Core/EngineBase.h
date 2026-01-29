#pragma once
#include "Core/D3DResourceLeakChecker.h"
#include "Core/SrvManager.h"
#include "Input/Input.h"
#include "Particle/ParticleManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/AbstractSceneFactory.h"
#include <memory>
#include <wrl.h>
#include <xaudio2.h>

class WindowSystem;
class Dx12Core;
class GameCamera;

class EngineBase {
public:
  // 仮想デストラクタ
  virtual ~EngineBase();

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

  Input *GetInputManager() const { return input_.get(); }
  SpriteRenderer *GetSpriteRenderer() const { return spriteRenderer_.get(); }
  Object3dRenderer *GetObject3dRenderer() const {
    return object3dRenderer_.get();
  }

protected:
  bool endRequest_ = false;

protected:
  std::unique_ptr<D3DResourceLeakChecker> leakChecker_ = nullptr;

  std::unique_ptr<WindowSystem> windowSystem_ = nullptr;

  std::unique_ptr<Dx12Core> dx12Core_ = nullptr;

  std::unique_ptr<Input> input_ = nullptr;

  Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;

  std::unique_ptr<SpriteRenderer> spriteRenderer_ = nullptr;

  std::unique_ptr<Object3dRenderer> object3dRenderer_ = nullptr;

  std::unique_ptr<SrvManager> srvManager_ = nullptr;

  // シーンファクトリー
  std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr;
};
