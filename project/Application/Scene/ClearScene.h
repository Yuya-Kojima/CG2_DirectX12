#pragma once
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
#include <memory>

class GameCamera;
class ParticleEmitter;
class Object3d;
class Sprite;

class ClearScene : public BaseScene {
private:
  // カメラ
  std::unique_ptr<GameCamera> camera_ = nullptr;
  Transform cameraTransform_{};

  std::unique_ptr<ParticleEmitter> clearFountain_ = nullptr;
  std::unique_ptr<ParticleEmitter> clearBurst_ = nullptr;
  std::unique_ptr<ParticleEmitter> clearGlitter_ = nullptr;
  bool didPlayClearBurst_ = false;
  float glitterManualTime_ = 0.0f;

  // クリア文字
  std::unique_ptr<Sprite> clearSprite_ = nullptr;
  Transform uvTransform_{};

  std::unique_ptr<Sprite> toTitle = nullptr;
  Transform uvToTitle_{};

  std::unique_ptr<Object3d> player_ = nullptr;
  Vector3 playerBasePos_{0.0f, 0.0f, 0.0f};
  float playerJumpTime_ = 0.0f;
  float jumpHeight_ = 0.6f;
  float jumpCycleSec_ = 0.7f;
  float groundHoldSec_ = 0.2f;

  // 天球（スカイドーム）
  std::unique_ptr<Object3d> skyObject3d_;
  float skyScale_ = 1.0f;
  float skyRotate_ = 0.0f;

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

  ~ClearScene();

private: // メンバ変数(システム用)
  // エンジン
  EngineBase *engine_ = nullptr;
};
