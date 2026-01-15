#pragma once
#include "Audio/SoundManager.h"
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
#include <vector>

class Sprite;
class Object3d;
class SpriteRenderer;
class Object3dRenderer;
class DebugCamera;
class InputKeyState;
class ParticleEmitter;

class TitleScene : public BaseScene {

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(EngineBase *engine) override;

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize() override;

  /// <summary>
  /// 更新
  /// </summary>
  void Update() override;

  /// <summary>
  /// 描画
  /// </summary>
  void Draw() override;

  void Draw3D() override;

  void Draw2D() override;

private:
  // SoundData soundData1_;

  static constexpr int kSpriteCount_ = 5;

  std::vector<Sprite *> sprites_;

  Sprite *sprite_ = nullptr;

  Vector2 spritePositions_[kSpriteCount_];

  Vector2 spriteSizes_[kSpriteCount_];

  // スプライトのTransform
  Vector2 spritePosition_{};

  float spriteRotation_{};

  Vector4 spriteColor_{};

  Vector2 spriteSize_{};

  Vector2 spriteAnchorPoint_{};

  bool isFlipX_ = false;
  bool isFlipY_ = false;

  Transform uvTransformSprite_{};

  Object3d *object3d_ = nullptr;
  Object3d *object3dA_ = nullptr;

  float rotateObj_{};

  ParticleEmitter *particleEmitter_ = nullptr;

  GameCamera *camera_ = nullptr;

  Transform cameraTransform_{};

  DebugCamera *debugCamera_ = nullptr;

  // エンジン
  EngineBase *engine_ = nullptr;
};
