#pragma once
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
#include <memory>
#include <vector>

class Sprite;

class SplashScene : public BaseScene {

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

  /// <summary>
  /// 2Dオブジェクト描画
  /// </summary>
  void Draw2D() override;

  /// <summary>
  /// 3Dオブジェクト描画
  /// </summary>
  void Draw3D() override;

private:
  EngineBase *engine_ = nullptr;
  
  std::unique_ptr<Sprite> logoSprite_ = nullptr;
  float time_ = 0.0f;
  float transitionTimer_ = 0.0f;
  const float displayDuration_ = 3.0f; // 3秒間表示してフェードアウト
  bool isTransitioning_ = false;
};
