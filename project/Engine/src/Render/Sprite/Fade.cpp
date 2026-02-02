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

  // 初期状態は透明
  state_ = State::Idle;
  alpha_ = 1.0f;
  target_ = 1.0f;
  speed_ = 0.0f;

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
}

void Fade::StartFadeOut(float durationSec) {
  if (!sprite_) {
    return;
  }

  durationSec = (std::max)(durationSec, 0.0001f);
  state_ = State::FadingOut;

  // 0→1 へ
  target_ = 1.0f;
  speed_ = 1.0f / durationSec;
}

void Fade::StartFadeIn(float durationSec) {
  if (!sprite_) {
    return;
  }

  durationSec = (std::max)(durationSec, 0.0001f);
  state_ = State::FadingIn;

  // 1→0 へ
  target_ = 0.0f;
  speed_ = 1.0f / durationSec;

  if (alpha_ <= 0.0f) {
    alpha_ = 1.0f;
    ApplyToSprite_();
  }
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

void Fade::ApplyToSprite_() {
  if (!sprite_) {
    return;
  }

  sprite_->SetColor(Vector4{0.0f, 0.0f, 0.0f, alpha_});
}