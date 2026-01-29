#pragma once
#include "Core/WindowSystem.h"
#include "Input/InputKeyState.h"
#include "InputMouseState.h"
#include "InputPadState.h"
#include <Xinput.h>
#include <array>

using KeyCode = InputKeyState::KeyCode;

using PadButton = InputPadState::PadButton;

class Input {

public:
  void Initialize(WindowSystem *windowSystem);
  void Update();

  // キーボード
  bool IsPressKey(BYTE dik) const { return keyboard_.IsPressKey(dik); }
  bool IsUpKey(BYTE dik) const { return keyboard_.IsUpKey(dik); }
  bool IsTriggerKey(BYTE dik) const { return keyboard_.IsTriggerKey(dik); }
  bool IsReleaseKey(BYTE dik) const { return keyboard_.IsReleaseKey(dik); }

  bool IsKeyPress(KeyCode key) const;
  bool IsKeyTrigger(KeyCode key) const;
  bool IsKeyRelease(KeyCode key) const;

  // マウス
  bool IsMousePress(int button) const { return mouse_.IsPress(button); }
  bool IsMouseTrigger(int button) const { return mouse_.IsTrigger(button); }
  bool IsMouseRelease(int button) const { return mouse_.IsRelease(button); }

  int GetMouseX() const { return mouse_.GetX(); }
  int GetMouseY() const { return mouse_.GetY(); }
  int GetMouseDeltaX() const { return mouse_.GetDeltaX(); }
  int GetMouseDeltaY() const { return mouse_.GetDeltaY(); }

  // PAD
  const InputPadState &Pad(int index = 0) const;

  // using PAD = InputPadState::PadButton;

  // bool IsPadPress(PAD button, int index = 0) const;
  // bool IsPadTrigger(PAD button, int index = 0) const;
  // bool IsPadRelease(PAD button, int index = 0) const;

  bool IsPadPress(PadButton pad, int index = 0) const;
  bool IsPadTrigger(PadButton pad, int index = 0) const;
  bool IsPadRelease(PadButton pad, int index = 0) const;

  Input(const Input &) = delete;
  Input &operator=(const Input &) = delete;
  Input() = default;

private:
  WindowSystem *windowSystem_ = nullptr;

  // キーボード
  InputKeyState keyboard_;

  // マウス
  InputMouseState mouse_;

  // PAD
  std::array<InputPadState, 4> pads_{};

private:
  void UpdateKeyboard_();
  void UpdateMouse_();
  void UpdatePad_();
};
