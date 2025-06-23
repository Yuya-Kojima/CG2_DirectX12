#include "InputKeyState.h"

void InputKeyState::Update(IDirectInputDevice8 *keyboard) {

  // 前の状態を保存
  memcpy(preKey, key, sizeof(key));

  // 現在
  HRESULT result = keyboard->GetDeviceState(sizeof(key), key);
  if (FAILED(result)) {
    keyboard->Acquire();
    keyboard->GetDeviceState(sizeof(key), key);
  }
}

bool InputKeyState::IsPressKey(BYTE dik) { return key[dik]; }

bool InputKeyState::IsUpKey(BYTE dik) { return !key[dik]; }

bool InputKeyState::IsTriggerKey(BYTE dik) { return key[dik] && !preKey[dik]; }

bool InputKeyState::IsReleaseKey(BYTE dik) { return !key[dik] && preKey[dik]; }
