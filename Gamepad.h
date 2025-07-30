#pragma once
#include <Windows.h>
#include <Xinput.h>

#pragma comment(lib, "xinput9_1_0.lib")

class Gamepad {
public:
  struct Stick {
    float x;
    float y;
  };

  static void Initialize();
  static void Update();
  static bool IsConnected(int index = 0);
  static bool IsButtonPressed(WORD button, int index = 0);
  static SHORT GetLeftStickX(int index = 0);
  static SHORT GetLeftStickY(int index = 0);
  static Stick GetNormalizedLeftStick(int index = 0);
  static Stick GetNormalizedRightStick(int index = 0);
  static float GetLeftTriggerStrength(int index = 0);
  static float GetRightTriggerStrength(int index = 0);
};