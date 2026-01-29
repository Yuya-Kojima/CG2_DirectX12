#pragma once
#include "Core/windowSystem.h"
#include <dinput.h>
#include <wrl.h>

class InputKeyState {
public:
  enum class KeyCode {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    Up,
    Down,
    Left,
    Right,

    Enter,
    Space,
    Escape,
    Tab,
    Shift,
    Control,

    Count
  };

  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

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

private:
  BYTE key_[256] = {};
  BYTE preKey_[256] = {};

  ComPtr<IDirectInputDevice8> keyboard_;
  WindowSystem *windowSystem_ = nullptr;
};
