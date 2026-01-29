#include "Input/InputMouseState.h"
#include <cassert>
#include <cstring>

void InputMouseState::Update(const char current[2], int x, int y) {

  // 前フレームの状態を保存
  std::memcpy(prevMouseButtons_, mouseButtons_, sizeof(mouseButtons_));
  prevX_ = x_;
  prevY_ = y_;

  // 今フレームの状態を更新
  std::memcpy(mouseButtons_, current, sizeof(mouseButtons_));
  x_ = x;
  y_ = y;
}

bool InputMouseState::IsPress(int button) const {
  assert(0 <= button && button < 2);
  return mouseButtons_[button] != 0;
}

bool InputMouseState::IsTrigger(int button) const {
  assert(0 <= button && button < 2);
  return (mouseButtons_[button] != 0) && (prevMouseButtons_[button] == 0);
}

bool InputMouseState::IsRelease(int button) const {
  assert(0 <= button && button < 2);
  return (mouseButtons_[button] == 0) && (prevMouseButtons_[button] != 0);
}