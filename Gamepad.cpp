#include "Gamepad.h"

XINPUT_STATE gInputState{};

void Gamepad::Initialize() { ZeroMemory(&gInputState, sizeof(XINPUT_STATE)); }

void Gamepad::Update() { XInputGetState(0, &gInputState); }

bool Gamepad::IsConnected(int index) {
  XINPUT_STATE state;
  return XInputGetState(index, &state) == ERROR_SUCCESS;
}

bool Gamepad::IsButtonPressed(WORD button, int index) {
  return (gInputState.Gamepad.wButtons & button) != 0;
}

SHORT Gamepad::GetLeftStickX(int index) { return gInputState.Gamepad.sThumbLX; }

SHORT Gamepad::GetLeftStickY(int index) { return gInputState.Gamepad.sThumbLY; }

static constexpr SHORT DEAD_ZONE = 7849;

Gamepad::Stick Gamepad::GetNormalizedLeftStick(int index) {
  Stick result = {0.0f, 0.0f};
  XINPUT_STATE state{};
  if (XInputGetState(index, &state) != ERROR_SUCCESS)
    return result;

  SHORT rawX = state.Gamepad.sThumbLX;
  SHORT rawY = state.Gamepad.sThumbLY;

  if (abs(rawX) < DEAD_ZONE && abs(rawY) < DEAD_ZONE)
    return result;

  result.x = static_cast<float>(rawX) / 32767.0f;
  result.y = static_cast<float>(rawY) / 32767.0f;
  return result;
}
Gamepad::Stick Gamepad::GetNormalizedRightStick(int index) {
  Stick result = {0.0f, 0.0f};
  XINPUT_STATE state{};
  if (XInputGetState(index, &state) != ERROR_SUCCESS)
    return result;

  SHORT rawX = state.Gamepad.sThumbRX;
  SHORT rawY = state.Gamepad.sThumbRY;

  if (abs(rawX) < DEAD_ZONE && abs(rawY) < DEAD_ZONE)
    return result;

  result.x = static_cast<float>(rawX) / 32767.0f;
  result.y = static_cast<float>(rawY) / 32767.0f;
  return result;
}

float Gamepad::GetLeftTriggerStrength(int index) {
  XINPUT_STATE state{};
  if (XInputGetState(index, &state) != ERROR_SUCCESS)
    return 0.0f;

  BYTE value = state.Gamepad.bLeftTrigger;
  if (value < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
    return 0.0f;

  return static_cast<float>(value) / 255.0f;
}

float Gamepad::GetRightTriggerStrength(int index) {
  XINPUT_STATE state{};
  if (XInputGetState(index, &state) != ERROR_SUCCESS)
    return 0.0f;

  BYTE value = state.Gamepad.bRightTrigger;
  if (value < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
    return 0.0f;

  return static_cast<float>(value) / 255.0f;
}