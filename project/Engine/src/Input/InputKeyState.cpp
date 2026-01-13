#include "Input/InputKeyState.h"
#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void InputKeyState::Initialize(WindowSystem *windowSystem) {

  this->windowSystem = windowSystem;

  HRESULT result;

  // DirectInputの初期化
  ComPtr<IDirectInput8> directInput = nullptr;
  result =
      DirectInput8Create(windowSystem->GetHInstance(), DIRECTINPUT_VERSION,
                         IID_IDirectInput8, (void **)&directInput, nullptr);
  assert(SUCCEEDED(result));

  // キーボードデバイスの生成
  keyboard = nullptr;
  result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
  assert(SUCCEEDED(result));

  // 入力データ形式のセット
  result = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
  assert(SUCCEEDED(result));

  // 排他制御レベルのセット
  result = keyboard->SetCooperativeLevel(
      windowSystem->GetHwnd(),
      DISCL_FOREGROUND // 画面が一番手前にある場合のみ入力を受け付ける
          | DISCL_NONEXCLUSIVE // デバイスをこのｎアプリだけで専有しない
          | DISCL_NOWINKEY // Windowsキーを無効化
  );
  assert(SUCCEEDED(result));
}

void InputKeyState::Update() {

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