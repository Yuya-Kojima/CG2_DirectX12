#pragma once
#include "Audio/SoundManager.h"
#include "Core/EngineBase.h"
#include "Debug/DebugCamera.h"
#include "Math/MathUtil.h"
#include "Object3d/Object3d.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/BillboardParticleEmitter.h"
#include "Particle/MeshParticleEmitter.h"
#include "Render/Renderer/SkyBoxRenderer.h"
#include "Render/SkyBox/SkyBox.h"
#include "Scene/BaseScene.h"
#include "Sprite/Sprite.h"
#include "Model/Skeleton.h"
#include <memory>
#include <vector>

class GameCamera;
class SpriteRenderer;
class Object3dRenderer;
class InputKeyState;

class DebugScene : public BaseScene {

private: // メンバ変数(ゲーム用)
  // カメラ
  std::unique_ptr<GameCamera> camera_ = nullptr;

  Transform cameraTransform_{};

  std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

  // デバッグカメラ
  bool useDebugCamera_ = false;

  static constexpr int kSpriteCount_ = 5;

  std::vector<std::unique_ptr<Sprite>> sprites_;

  std::unique_ptr<Sprite> sprite_ = nullptr;

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

  std::unique_ptr<Object3d> object3d_ = nullptr;
  std::unique_ptr<Object3d> object3dA_ = nullptr;

  std::unique_ptr<Object3d> animatedCube_ = nullptr;
  Animation animation_;

  std::unique_ptr<Model> sneakWalkModel_ = nullptr;
  Animation sneakWalkAnimation_;
  float sneakWalkAnimationTime_ = 0.0f;
  Skeleton skeleton_;
  std::unique_ptr<Model> sphereModel_ = nullptr;
  std::vector<std::unique_ptr<Object3d>> jointObjects_;

  float rotateObj_{};

  std::unique_ptr<BillboardParticleEmitter> testParticleGroup_;
  std::unique_ptr<BillboardParticleEmitter> clearParticleGroup_;
  std::unique_ptr<MeshParticleEmitter> hitParticleGroup_;
  std::unique_ptr<MeshParticleEmitter> cylinderParticleGroup_;
  std::unique_ptr<MeshParticleEmitter> planeHitParticleGroup_;

  std::unique_ptr<ParticleEmitter> particleEmitter_ = nullptr;
  std::unique_ptr<ParticleEmitter> clearParticleEmitter_ = nullptr;
  std::unique_ptr<ParticleEmitter> hitParticleEmitter_ = nullptr;
  std::unique_ptr<ParticleEmitter> cylinderEmitter_ = nullptr;
  std::unique_ptr<ParticleEmitter> planeHitParticleEmitter_ = nullptr;

  // スカイボックス
  std::unique_ptr<Skybox> skybox_ = nullptr;

public: // メンバ関数
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

  ~DebugScene() override;

private: // メンバ変数(システム用)
  // エンジン
  EngineBase *engine_ = nullptr;
};
