#pragma once
#include "Core/Dx12Core.h"
#include "Core/SrvManager.h"
#include "Render/Texture/RenderTexture.h"
#include "Render/Renderer/Bloom.h"
#include "Renderer/PostProcess.h"
#include <memory>

class RenderPipeline {
public:
  RenderPipeline() = default;
  ~RenderPipeline() = default;

  // 初期化（レンダーターゲットやブルームの生成）
  void Initialize(Dx12Core* dx12Core, SrvManager* srvManager);

  // 3D描画パスの開始（HDRキャンバスをターゲットにセット）
  void Begin3DPass(Dx12Core* dx12Core);

  // ポストプロセス処理（Bloom, モーションブラー等）を実行し、メイン画面へ焼き付ける
  void DrawPostProcess(Dx12Core* dx12Core, SrvManager* srvManager, PostProcess* currentScenePostProcess);

private:
  std::unique_ptr<RenderTexture> mainRenderTexture_ = nullptr;
  std::unique_ptr<Bloom> bloom_ = nullptr;
  std::unique_ptr<RenderTexture> historyTextures_[2] = { nullptr, nullptr };
  
  uint32_t currentHistoryIndex_ = 0;
  uint32_t depthTextureSrvIndex_ = 0;
};
