#pragma once
#include <Windows.h>
#include <cstdint>

#pragma comment(lib, "winmm.lib")

class WindowSystem {

private:
  HWND hwnd = nullptr;

  WNDCLASS wc{};

public:
  // 静的メンバ関数
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam,
                                     LPARAM lparam);
  // クライアント領域のサイズ 静的
  static const int32_t kClientWidth = 1280;
  static const int32_t kClientHeight = 720;

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize();

  /// <summary>
  /// 更新
  /// </summary>
  void Update();

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

  /// <summary>
  /// hwndゲッター
  /// </summary>
  /// <returns></returns>
  HWND GetHwnd() const { return hwnd; }

  /// <summary>
  /// hInstanceゲッター
  /// </summary>
  /// <returns></returns>
  HINSTANCE GetHInstance() const { return wc.hInstance; }

  /// <summary>
  /// メッセージ処理
  /// </summary>
  /// <returns></returns>
  bool ProcessMessage();
};
