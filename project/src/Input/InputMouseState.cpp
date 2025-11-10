#include "Input/InputMouseState.h"
#include <cstring>  

void InputMouseState::Update(const char current[2]) {

  // 前フレームの状態を保存
  std::memcpy(prevMouseButtons_, mouseButtons_, sizeof(mouseButtons_));

      // 今フレームの状態を更新
  std::memcpy(mouseButtons_, current, sizeof(mouseButtons_));
}

bool InputMouseState::IsPress(int button) const {
  return mouseButtons_[button];
}

bool InputMouseState::IsTrigger(int button) const {
  return mouseButtons_[button] && !prevMouseButtons_[button];
}

bool InputMouseState::IsRelease(int button) const {
  return !mouseButtons_[button] && prevMouseButtons_[button];
}