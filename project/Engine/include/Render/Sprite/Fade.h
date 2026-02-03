#pragma once
#include "Math/MathUtil.h"
#include <memory>
#include <string>

class Sprite;
class SpriteRenderer;

class Fade {

public:
  enum class FadeType {
    Solid,
    WipeLeft,
    WipeRight,
    WipeUp,
    WipeDown,
  };

  enum class EasingType {
    Linear,
    EaseOutCubic,
    EaseInOutCubic,
    EaseOutQuint,
  };

  void Initialize(SpriteRenderer *spriteRenderer,
                  const std::string &texturePath = "resources/white1x1.png");

  void Finalize();

  void Update(float deltaTime);

  void Draw();

  // デフォルトフェード
  void StartFadeOut(float durationSec);
  void StartFadeIn(float durationSec);

  // タイプ、色指定、イージング
  void StartFadeOut(float durationSec, FadeType type, const Vector4 &color,
                    EasingType easing);

  void StartFadeIn(float durationSec, FadeType type, const Vector4 &color,
                   EasingType easing);

  bool IsPlaying() const { return state_ != State::Idle; }
  bool IsFinished() const { return state_ == State::Idle; }

  bool IsFading() const { return state_ != State::Idle; }
  bool IsCovered() const { return (state_ == State::Idle) && (alpha_ >= 1.0f); }
  bool IsClear() const { return (state_ == State::Idle) && (alpha_ <= 0.0f); }

  Fade();
  ~Fade();

private:
  enum class State {
    Idle,
    FadingOut,
    FadingIn,
  };

  FadeType type_ = FadeType::Solid;
  EasingType easing_ = EasingType::EaseOutCubic;

  void ApplySolid_(float eased);
  void ApplyWipeLeft_(float eased);
  void ApplyWipeRight_(float eased);
  void ApplyWipeUp_(float eased);
  void ApplyWipeDown_(float eased);
  void ApplyToSprite_();
  static float ApplyEasing_(float t, EasingType easing);
  void ApplyByType_(float eased);

  SpriteRenderer *spriteRenderer_ = nullptr;
  std::unique_ptr<Sprite> sprite_ = nullptr;

  std::string texturePath_;

  State state_ = State::Idle;

  Vector4 color_{};

  float alpha_ = 0.0f;
  float speed_ = 1.0f;
  float target_ = 0.0f;
};
