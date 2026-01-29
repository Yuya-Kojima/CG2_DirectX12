#include "Input/InputKeyState.h"
#include <cassert>
#include <cstring>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void InputKeyState::Initialize(WindowSystem *windowSystem) {

  this->windowSystem_ = windowSystem;

  HRESULT result;

  // DirectInputの初期化
  ComPtr<IDirectInput8> directInput = nullptr;
  result =
      DirectInput8Create(windowSystem->GetHInstance(), DIRECTINPUT_VERSION,
                         IID_IDirectInput8, (void **)&directInput, nullptr);
  assert(SUCCEEDED(result));

  // キーボードデバイスの生成
  keyboard_ = nullptr;
  result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
  assert(SUCCEEDED(result));

  // 入力データ形式のセット
  result = keyboard_->SetDataFormat(&c_dfDIKeyboard); // 標準形式
  assert(SUCCEEDED(result));

  // 排他制御レベルのセット
  result = keyboard_->SetCooperativeLevel(
      windowSystem->GetHwnd(),
      DISCL_FOREGROUND         // 画面が一番手前にある場合のみ入力を受け付ける
          | DISCL_NONEXCLUSIVE // デバイスをこのｎアプリだけで専有しない
          | DISCL_NOWINKEY     // Windowsキーを無効化
  );
  assert(SUCCEEDED(result));
}

void InputKeyState::Update() {

  // 前のキーボード状態を取得
  std::memcpy(preKey_, key_, sizeof(key_));

  // 現在のキーボード状態
  HRESULT result = keyboard_->GetDeviceState(sizeof(key_), key_);
  if (FAILED(result)) {
    result = keyboard_->Acquire();
    if (SUCCEEDED(result)) {
      result = keyboard_->GetDeviceState(sizeof(key_), key_);
    }
    if (FAILED(result)) {
      std::memset(key_, 0, sizeof(key_));
    }
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
