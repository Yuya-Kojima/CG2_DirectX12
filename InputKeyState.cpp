#include "InputKeyState.h"

void InputKeyState::Update(IDirectInputDevice8 *keyboard) {

  // 前のキーボード状態を取得
  memcpy(preKey_, key_, sizeof(key_));

  // 現在のキーボード状態
  HRESULT result = keyboard->GetDeviceState(sizeof(key_), key_);
  if (FAILED(result)) {
    keyboard->Acquire();
    keyboard->GetDeviceState(sizeof(key_), key_);
  }

  // マウスボタン状態の取得（GetAsyncKeyState を使用）
  memcpy(preMouse_, mouse_, sizeof(mouse_));
  mouse_[0] = (GetAsyncKeyState(VK_LBUTTON)) ? 1 : 0; // 左
  mouse_[1] = (GetAsyncKeyState(VK_RBUTTON)) ? 1 : 0; // 右

  // マウス座標の取得
  prevMouseX_ = mouseX_;
  prevMouseY_ = mouseY_;

  POINT p{};
  if (GetCursorPos(&p)) {
    mouseX_ = p.x;
    mouseY_ = p.y;
  }
}

bool InputKeyState::IsPressKey(BYTE dik) const { return key_[dik]; }

bool InputKeyState::IsUpKey(BYTE dik) const { return !key_[dik]; }

bool InputKeyState::IsTriggerKey(BYTE dik) const {
  return key_[dik] && !preKey_[dik];
}

bool InputKeyState::IsReleaseKey(BYTE dik) const {
  return !key_[dik] && preKey_[dik];
}

bool InputKeyState::IsMousePress(int button) const { return mouse_[button]; }

bool InputKeyState::IsMouseTrigger(int button) const {
  return mouse_[button] && !preMouse_[button];
}

bool InputKeyState::IsMouseRelease(int button) const {
  return !mouse_[button] && preMouse_[button];
}