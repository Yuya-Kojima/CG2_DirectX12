#pragma once
#include <cstring>
#include <dinput.h>

class InputKeyState {

private:
  BYTE key[256] = {};
  BYTE preKey[256] = {};

public:
  /// <summary>
  /// 更新処理
  /// </summary>
  /// <param name="keyboard"></param>
  void Update(IDirectInputDevice8 *keyboard);

  /// <summary>
  /// プレス
  /// </summary>
  /// <param name="dik"></param>
  /// <returns></returns>
  bool IsPressKey(BYTE dik);

  /// <summary>
  /// 離してる状態
  /// </summary>
  /// <param name="dik"></param>
  /// <returns></returns>
  bool IsUpKey(BYTE dik);

  /// <summary>
  /// トリガー
  /// </summary>
  /// <param name="dik"></param>
  /// <returns></returns>
  bool IsTriggerKey(BYTE dik);

  /// <summary>
  /// 離した瞬間
  /// </summary>
  /// <param name="dik"></param>
  /// <returns></returns>
  bool IsReleaseKey(BYTE dik);
};
