#pragma once
#include <memory>
#include <string>

class Sprite;
class SpriteRenderer;

class Fade {

public:
  void Initialize(SpriteRenderer *spriteRenderer,
                  const std::string &texturePath = "resources/white1x1.png");

  void Finalize();

  void Update(float deltaTime);

  void Draw();

  void StartFadeOut(float durationSec);

  void StartFadeIn(float durationSec);

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

  void ApplyToSprite_();

  SpriteRenderer *spriteRenderer_ = nullptr;
  std::unique_ptr<Sprite> sprite_ = nullptr;

  std::string texturePath_;

  State state_ = State::Idle;

  float alpha_ = 0.0f;
  float speed_ = 1.0f;
  float target_ = 0.0f;
};
