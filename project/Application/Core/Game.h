#pragma once
#include "Core/EngineBase.h"
#include "Debug/ImGuiManager.h"
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
#include "Renderer/PostProcess.h"
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
  std::unique_ptr<PostProcess> postProcess_ = nullptr;

  uint32_t renderTextureSrvIndex_ = 0;
};
