#pragma once
#include "Core/EngineBase.h"
#include "GamePlayScene.h"
#include "Math/MathUtil.h"
#include <vector>

class Sprite;
class Object3d;

class Game : public EngineBase {

public:
  void Initialize() override;

  void Finalize() override;

  void Update() override;

  void Draw() override;

private:
  GamePlayScene *scene_ = nullptr;

  bool set60FPS_ = false;
};
