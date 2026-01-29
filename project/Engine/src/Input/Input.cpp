#include "Input/Input.h"
#include <cassert>

#pragma comment(lib, "xinput.lib")

namespace {
BYTE ToDIK(KeyCode k) {
  switch (k) {
  case KeyCode::A:
    return DIK_A;
  case KeyCode::B:
    return DIK_B;
  case KeyCode::C:
    return DIK_C;
  case KeyCode::D:
    return DIK_D;
  case KeyCode::E:
    return DIK_E;
  case KeyCode::F:
    return DIK_F;
  case KeyCode::G:
    return DIK_G;
  case KeyCode::H:
    return DIK_H;
  case KeyCode::I:
    return DIK_I;
  case KeyCode::J:
    return DIK_J;
  case KeyCode::K:
    return DIK_K;
  case KeyCode::L:
    return DIK_L;
  case KeyCode::M:
    return DIK_M;
  case KeyCode::N:
    return DIK_N;
  case KeyCode::O:
    return DIK_O;
  case KeyCode::P:
    return DIK_P;
  case KeyCode::Q:
    return DIK_Q;
  case KeyCode::R:
    return DIK_R;
  case KeyCode::S:
    return DIK_S;
  case KeyCode::T:
    return DIK_T;
  case KeyCode::U:
    return DIK_U;
  case KeyCode::V:
    return DIK_V;
  case KeyCode::W:
    return DIK_W;
  case KeyCode::X:
    return DIK_X;
  case KeyCode::Y:
    return DIK_Y;
  case KeyCode::Z:
    return DIK_Z;

  case KeyCode::Up:
    return DIK_UP;
  case KeyCode::Down:
    return DIK_DOWN;
  case KeyCode::Left:
    return DIK_LEFT;
  case KeyCode::Right:
    return DIK_RIGHT;

  case KeyCode::Enter:
    return DIK_RETURN;
  case KeyCode::Space:
    return DIK_SPACE;
  case KeyCode::Escape:
    return DIK_ESCAPE;
  case KeyCode::Tab:
    return DIK_TAB;
  case KeyCode::Shift:
    return DIK_LSHIFT;
  case KeyCode::Control:
    return DIK_LCONTROL;
  default:
    return 0;
  }
}
} // namespace

namespace {

// short [-32768,32767] を deadzone 付きで [-1,1] に正規化
inline float NormalizeStick(SHORT v, SHORT deadzone) {
  int vi = static_cast<int>(v);
  int av = (vi < 0) ? -vi : vi;
  if (av <= deadzone)
    return 0.0f;

  // deadzone を除いた範囲で [0,1]
  const float sign = (vi < 0) ? -1.0f : 1.0f;
  const float norm = (static_cast<float>(av - deadzone)) /
                     (32767.0f - static_cast<float>(deadzone));
  return sign * (norm > 1.0f ? 1.0f : norm);
}

inline float NormalizeTrigger(BYTE v, BYTE threshold) {
  if (v <= threshold)
    return 0.0f;
  return (static_cast<float>(v - threshold)) /
         (255.0f - static_cast<float>(threshold));
}

inline void FillButtonsFromXInput(
    WORD wButtons,
    std::array<bool, static_cast<size_t>(InputPadState::PadButton::Count)>
        &out) {
  out.fill(false);

  using B = InputPadState::PadButton;

  out[static_cast<size_t>(B::A)] = (wButtons & XINPUT_GAMEPAD_A) != 0;
  out[static_cast<size_t>(B::B)] = (wButtons & XINPUT_GAMEPAD_B) != 0;
  out[static_cast<size_t>(B::X)] = (wButtons & XINPUT_GAMEPAD_X) != 0;
  out[static_cast<size_t>(B::Y)] = (wButtons & XINPUT_GAMEPAD_Y) != 0;

  out[static_cast<size_t>(B::LB)] =
      (wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
  out[static_cast<size_t>(B::RB)] =
      (wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;

  out[static_cast<size_t>(B::Back)] = (wButtons & XINPUT_GAMEPAD_BACK) != 0;
  out[static_cast<size_t>(B::Start)] = (wButtons & XINPUT_GAMEPAD_START) != 0;

  out[static_cast<size_t>(B::LS)] = (wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
  out[static_cast<size_t>(B::RS)] =
      (wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;

  out[static_cast<size_t>(B::DPadUp)] =
      (wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
  out[static_cast<size_t>(B::DPadDown)] =
      (wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
  out[static_cast<size_t>(B::DPadLeft)] =
      (wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
  out[static_cast<size_t>(B::DPadRight)] =
      (wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
}

} // namespace

void Input::Initialize(WindowSystem *windowSystem) {
  assert(windowSystem);
  windowSystem_ = windowSystem;

  keyboard_.Initialize(windowSystem_);

  // HRESULT hr;

  // DirectInput 作成
  // ComPtr<IDirectInput8> directInput = nullptr;
  // hr = DirectInput8Create(windowSystem_->GetHInstance(), DIRECTINPUT_VERSION,
  //                        IID_IDirectInput8, (void **)&directInput, nullptr);
  // assert(SUCCEEDED(hr));

  // キーボードデバイス
  // keyboard_ = nullptr;
  // hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard_, nullptr);
  // assert(SUCCEEDED(hr));

  // hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
  // assert(SUCCEEDED(hr));

  // hr = keyboard_->SetCooperativeLevel(windowSystem_->GetHwnd(),
  //                                     DISCL_FOREGROUND | DISCL_NONEXCLUSIVE |
  //                                         DISCL_NOWINKEY);
  // assert(SUCCEEDED(hr));
}

void Input::Update() {
  assert(windowSystem_);
  UpdateKeyboard_();
  UpdateMouse_();
  UpdatePad_();
}

const InputPadState &Input::Pad(int index) const {
  assert(0 <= index && index < static_cast<int>(pads_.size()));
  return pads_[index];
}

bool Input::IsKeyPress(KeyCode key) const {
  const BYTE dik = ToDIK(key);
  return (dik != 0) ? keyboard_.IsPressKey(dik) : false;
}
bool Input::IsKeyTrigger(KeyCode key) const {
  const BYTE dik = ToDIK(key);
  return (dik != 0) ? keyboard_.IsTriggerKey(dik) : false;
}
bool Input::IsKeyRelease(KeyCode key) const {
  const BYTE dik = ToDIK(key);
  return (dik != 0) ? keyboard_.IsReleaseKey(dik) : false;
}

bool Input::IsPadPress(PadButton button, int index) const {
  assert(0 <= index && index < (int)pads_.size());
  return pads_[index].IsPress(button);
}
bool Input::IsPadTrigger(PadButton button, int index) const {
  assert(0 <= index && index < (int)pads_.size());
  return pads_[index].IsTrigger(button);
}
bool Input::IsPadRelease(PadButton button, int index) const {
  assert(0 <= index && index < (int)pads_.size());
  return pads_[index].IsRelease(button);
}

void Input::UpdateKeyboard_() {

  keyboard_.Update();

  //// 前フレーム保存
  // std::memcpy(preKey_.data(), key_.data(), key_.size());

  // HRESULT hr = keyboard_->GetDeviceState((DWORD)key_.size(), key_.data());
  // if (FAILED(hr)) {
  //   hr = keyboard_->Acquire();
  //   if (SUCCEEDED(hr)) {
  //     hr = keyboard_->GetDeviceState((DWORD)key_.size(), key_.data());
  //   }
  //   if (FAILED(hr)) {
  //     std::memset(key_.data(), 0, key_.size());
  //   }
  // }
}

void Input::UpdateMouse_() {
  char buttons[2];
  buttons[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;
  buttons[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 1 : 0;

  POINT p{};
  GetCursorPos(&p);

  mouse_.Update(buttons, p.x, p.y);
}

void Input::UpdatePad_() {
  using B = InputPadState::PadButton;
  constexpr SHORT kDeadL = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
  constexpr SHORT kDeadR = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
  constexpr BYTE kTrigT = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

  for (DWORD i = 0; i < 4; ++i) {
    XINPUT_STATE state{};
    const DWORD res = XInputGetState(i, &state);
    const bool connected = (res == ERROR_SUCCESS);

    std::array<bool, static_cast<size_t>(B::Count)> buttons{};
    float lx = 0, ly = 0, rx = 0, ry = 0, lt = 0, rt = 0;

    if (connected) {
      const auto &g = state.Gamepad;

      FillButtonsFromXInput(g.wButtons, buttons);

      lx = NormalizeStick(g.sThumbLX, kDeadL);
      ly = NormalizeStick(g.sThumbLY, kDeadL);
      rx = NormalizeStick(g.sThumbRX, kDeadR);
      ry = NormalizeStick(g.sThumbRY, kDeadR);

      lt = NormalizeTrigger(g.bLeftTrigger, kTrigT);
      rt = NormalizeTrigger(g.bRightTrigger, kTrigT);
    }

    pads_[i].Update(buttons, lx, ly, rx, ry, lt, rt, connected);
  }
}
