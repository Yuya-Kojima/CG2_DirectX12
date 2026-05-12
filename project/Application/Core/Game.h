#pragma once
#include "Core/EngineBase.h"
#include "Debug/ImGuiManager.h"
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
#include "Renderer/PostProcess.h"
#include "Render/Texture/RenderTexture.h"
#include "Render/Renderer/Bloom.h"
#include <memory>
#include <vector>

class ImGuiManager;

class Game : public EngineBase {

public:
  void Initialize() override;

  void Finalize() override;

  void Update() override;

  void Draw() override;

private:
  bool set60FPS_ = true;

  std::unique_ptr<ImGuiManager> imGuiManager_ = nullptr;

  std::unique_ptr<RenderTexture> mainRenderTexture_ = nullptr;
  std::unique_ptr<Bloom> bloom_ = nullptr;
  uint32_t depthTextureSrvIndex_ = 0;
};
