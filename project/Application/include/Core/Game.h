#pragma once
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include <vector>

class Sprite;
class Object3d;
class ParticleEmitter;
class DebugCamera;

class Game : public EngineBase {

public:
  void Initialize() override;

  void Finalize() override;

  void Update() override;

  void Draw() override;

private:
  SoundData soundData1_;

  Sprite *sprite_ = nullptr;

  static constexpr int kSpriteCount_ = 5;

  std::vector<Sprite *> sprites_;

  Vector2 spritePositions_[kSpriteCount_];

  Vector2 spriteSizes_[kSpriteCount_];

  GameCamera *camera_ = nullptr;

  Object3d *object3d_ = nullptr;
  Object3d *object3dA_ = nullptr;

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
