#pragma once
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
#include <vector>

class Game : public EngineBase {

public:
  void Initialize() override;

  void Finalize() override;

  void Update() override;

  void Draw() override;

private:
  bool set60FPS_ = false;
};
