#pragma once
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
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

  ImGuiManager *imGuiManager_ = nullptr;
};
