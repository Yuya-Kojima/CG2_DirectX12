#pragma once
#include "windowSystem.h"
#include <Windows.h>
#include <cstring>
#include <dinput.h>
#include <wrl.h>

class InputKeyState {

public:
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
  BYTE key_[256] = {};
  BYTE preKey_[256] = {};

  char mouse_[2]{};
  char preMouse_[2]{};

  int mouseX_ = 0;
  int mouseY_ = 0;
  int prevMouseX_ = 0;
  int prevMouseY_ = 0;

  ComPtr<IDirectInputDevice8> keyboard;

  WindowSystem *windowSystem = nullptr;

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(WindowSystem *windowSystem);

  /// <summary>
  /// 更新処理
  /// </summary>
  /// <param name="keyboard"></param>
  void Update();

  /// <summary>
  /// プレス
  /// </summary>
  /// <param name="dik"></param>
  /// <returns></returns>
  bool IsPressKey(BYTE dik) const;

  /// <summary>
  /// 離してる状態
  /// </summary>
  /// <param name="dik"></param>
  /// <returns></returns>
  bool IsUpKey(BYTE dik) const;

  /// <summary>
  /// トリガー
  /// </summary>
  /// <param name="dik"></param>
  /// <returns></returns>
  bool IsTriggerKey(BYTE dik) const;

  /// <summary>
  /// 離した瞬間
  /// </summary>
  /// <param name="dik"></param>
  /// <returns></returns>
  bool IsReleaseKey(BYTE dik) const;

  /// <summary>
  /// マウスを
  /// </summary>
  /// <param name="button"></param>
  /// <returns></returns>
  bool IsMousePress(int button) const;

  bool IsMouseTrigger(int button) const;

  bool IsMouseRelease(int button) const;
};
