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
  };

  void Initialize(SpriteRenderer *spriteRenderer,
                  const std::string &texturePath = "resources/white1x1.png");

  void Finalize();

  void Update(float deltaTime);

  void Draw();

  void StartFadeOut(float durationSec);
  void StartFadeIn(float durationSec);

  void StartFadeOut(float durationSec, FadeType type, const Vector4 &color);
  void StartFadeIn(float durationSec, FadeType type, const Vector4 &color);

  bool IsPlaying() const { return state_ != State::Idle; }
  bool IsFinished() const { return state_ == State::Idle; }

  bool IsFading() const { return state_ != State::Idle; }
  bool IsBlack() const { return (state_ == State::Idle) && (alpha_ >= 1.0f); }
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

  void ApplySolid_();
  void ApplyWipeLeft_();
  void ApplyToSprite_();

  SpriteRenderer *spriteRenderer_ = nullptr;
  std::unique_ptr<Sprite> sprite_ = nullptr;

  std::string texturePath_;

  State state_ = State::Idle;

  Vector4 color_{};

  float alpha_ = 0.0f;
  float speed_ = 1.0f;
  float target_ = 0.0f;
};
