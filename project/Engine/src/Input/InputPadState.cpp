#include "Input/InputPadState.h"

#include <cassert>

namespace {
constexpr size_t ToIndex(InputPadState::PadButton b) {
  return static_cast<size_t>(b);
}
} // namespace

void InputPadState::Update(
    const std::array<bool, static_cast<size_t>(PadButton::Count)> &current,
    float lx, float ly, float rx, float ry, float lt, float rt,
    bool connected) {

  prevButtons_ = buttons_;
  buttons_ = current;

  lx_ = lx;
  ly_ = ly;
  rx_ = rx;
  ry_ = ry;
  lt_ = lt;
  rt_ = rt;

  connected_ = connected;
}

bool InputPadState::IsPress(PadButton button) const {
  const size_t i = ToIndex(button);
  assert(i < buttons_.size());
  return buttons_[i];
}

bool InputPadState::IsTrigger(PadButton button) const {
  const size_t i = ToIndex(button);
  assert(i < buttons_.size());
  return buttons_[i] && !prevButtons_[i];
}

bool InputPadState::IsRelease(PadButton button) const {
  const size_t i = ToIndex(button);
  assert(i < buttons_.size());
  return !buttons_[i] && prevButtons_[i];
}
