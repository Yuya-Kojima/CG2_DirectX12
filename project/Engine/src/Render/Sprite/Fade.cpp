#include "Sprite/Fade.h"
#include "Debug/Logger.h"
#include "Renderer/SpriteRenderer.h"
#include "Sprite/Sprite.h"

#include <algorithm>
#include <cassert>

void Fade::Initialize(SpriteRenderer *spriteRenderer,
                      const std::string &texturePath) {

  spriteRenderer_ = spriteRenderer;
  texturePath_ = texturePath;

  assert(spriteRenderer_ && "Fade::Initialize: spriteRenderer is null");

  sprite_ = std::make_unique<Sprite>();

  sprite_->Initialize(spriteRenderer_, texturePath_);

  sprite_->SetAnchorPoint({0.0f, 0.0f});
  sprite_->SetPosition({0.0f, 0.0f});
  sprite_->SetSize(
      {float(WindowSystem::kClientWidth), float(WindowSystem::kClientHeight)});

  state_ = State::Idle;
  alpha_ = 1.0f;
  target_ = 1.0f;
  speed_ = 0.0f;

  // 黒で初期化
  color_ = Vector4{0.0f, 0.0f, 0.0f, 1.0f};

  ApplyToSprite_();
}

void Fade::Finalize() {
  sprite_.reset();
  spriteRenderer_ = nullptr;
  texturePath_.clear();
  state_ = State::Idle;
  alpha_ = 0.0f;
  target_ = 0.0f;
  speed_ = 1.0f;
  color_ = Vector4{0.0f, 0.0f, 0.0f, 1.0f};
}

// void Fade::StartFadeOut(float durationSec) {
//   if (!sprite_) {
//     return;
//   }
//
//   durationSec = (std::max)(durationSec, 0.0001f);
//   state_ = State::FadingOut;
//
//   // 0→1 へ
//   target_ = 1.0f;
//   speed_ = 1.0f / durationSec;
// }
//
// void Fade::StartFadeIn(float durationSec) {
//   if (!sprite_) {
//     return;
//   }
//
//   durationSec = (std::max)(durationSec, 0.0001f);
//   state_ = State::FadingIn;
//
//   // 1→0 へ
//   target_ = 0.0f;
//   speed_ = 1.0f / durationSec;
//
//   if (alpha_ <= 0.0f) {
//     alpha_ = 1.0f;
//     ApplyToSprite_();
//   }
// }

void Fade::StartFadeOut(float durationSec, FadeType type,
                        const Vector4 &color) {
  if (!sprite_)
    return;

  type_ = type;
  color_ = color;

  durationSec = (std::max)(durationSec, 0.0001f);
  state_ = State::FadingOut;
  target_ = 1.0f;
  speed_ = 1.0f / durationSec;

  ApplyToSprite_();
}

void Fade::StartFadeIn(float durationSec, FadeType type, const Vector4 &color) {

  if (!sprite_)
    return;

  type_ = type;
  color_ = color;

  durationSec = (std::max)(durationSec, 0.0001f);
  state_ = State::FadingIn;
  target_ = 0.0f;
  speed_ = 1.0f / durationSec;

  if (alpha_ <= 0.0f) {
    alpha_ = 1.0f;
  }

  ApplyToSprite_();
}

void Fade::StartFadeOut(float durationSec) {
  StartFadeOut(durationSec, type_, Vector4{0, 0, 0, 1});
}

void Fade::StartFadeIn(float durationSec) {
  StartFadeIn(durationSec, type_, Vector4{0, 0, 0, 1});
}

void Fade::Update(float deltaTime) {
  if (!sprite_) {
    return;
  }

  Transform uv{};
  uv.scale = {1.0f, 1.0f, 1.0f};
  uv.rotate = {0.0f, 0.0f, 0.0f};
  uv.translate = {0.0f, 0.0f, 0.0f};
  sprite_->Update(uv);

  if (state_ == State::Idle) {
    return;
  }

  // 方向に応じて alpha を動かす
  if (state_ == State::FadingOut) {
    alpha_ += speed_ * deltaTime;
    if (alpha_ >= target_) {
      alpha_ = target_;
      state_ = State::Idle;
    }
  } else if (state_ == State::FadingIn) {
    alpha_ -= speed_ * deltaTime;
    if (alpha_ <= target_) {
      alpha_ = target_;
      state_ = State::Idle;
    }
  }

  alpha_ = std::clamp(alpha_, 0.0f, 1.0f);
  ApplyToSprite_();
}

void Fade::Draw() {
  if (!sprite_) {
    return;
  }

  if (alpha_ <= 0.0f) {
    return;
  }

  sprite_->Draw();
}

Fade::Fade() = default;

Fade::~Fade() = default;

void Fade::ApplySolid_() {
  Vector4 c = color_;
  c.w = alpha_;
  sprite_->SetColor(c);

  // フルスクリーン
  sprite_->SetPosition({0.0f, 0.0f});
  sprite_->SetSize(
      {float(WindowSystem::kClientWidth), float(WindowSystem::kClientHeight)});
}

void Fade::ApplyWipeLeft_() {
  Vector4 c = color_;
  c.w = 1.0f;
  sprite_->SetColor(c);

  float width = WindowSystem::kClientWidth * alpha_;

  sprite_->SetPosition({0.0f, 0.0f});
  sprite_->SetSize({width, float(WindowSystem::kClientHeight)});
}

void Fade::ApplyToSprite_() {
  if (!sprite_) {
    return;
  }

  switch (type_) {
  case FadeType::Solid:
    ApplySolid_();
    break;

  case FadeType::WipeLeft:
    ApplyWipeLeft_();
    break;
  }

  Vector4 c = color_;
  c.w = alpha_;
  sprite_->SetColor(c);
}