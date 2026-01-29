#pragma once
#include <array>
#include <cstddef>

class InputPadState {

public:
  enum class PadButton {
    A,
    B,
    X,
    Y,
    LB,
    RB,
    Back,
    Start,
    LS,
    RS,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    Count
  };

  void
  Update(const std::array<bool, static_cast<size_t>(PadButton::Count)> &current,
         float lx, float ly, float rx, float ry, float lt, float rt,
         bool connected);

  bool IsPress(PadButton button) const;
  bool IsTrigger(PadButton button) const;
  bool IsRelease(PadButton button) const;

  bool IsConnected() const { return connected_; }

  float GetLeftX() const { return lx_; }
  float GetLeftY() const { return ly_; }
  float GetRightX() const { return rx_; }
  float GetRightY() const { return ry_; }
  float GetLT() const { return lt_; }
  float GetRT() const { return rt_; }

private:
  std::array<bool, static_cast<size_t>(PadButton::Count)> buttons_{};
  std::array<bool, static_cast<size_t>(PadButton::Count)> prevButtons_{};

  float lx_ = 0.0f;
  float ly_ = 0.0f;
  float rx_ = 0.0f;
  float ry_ = 0.0f;
  float lt_ = 0.0f;
  float rt_ = 0.0f;

  bool connected_ = false;
};
