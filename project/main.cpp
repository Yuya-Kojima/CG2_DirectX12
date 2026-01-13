#include "Math/MathUtil.h"
#include "Math/Matrix4x4.h"
#include "Math/Transform.h"
#include "Math/Vector4.h"
#include "Math/VertexData.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
// #include <Windows.h>

#include <cstdint>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <format>
#include <string>
#define _USE_MATH_DEFINES
#include "Core/Dx12Core.h"
#include "Core/Game.h"
#include "Core/Logger.h"
#include "Math/Field.h"
#include "Math/TransformationMatrix.h"
#include "Render/DirectionalLight.h"
#include "Render/Material.h"
#include "Render/ModelData.h"

#include "Util/StringUtil.h"
#include <dinput.h>
#include <math.h>
#include <numbers>
#include <random>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include "Core/EngineBase.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  // ゲームの初期化
  EngineBase *game = new Game();

  game->Run();

  // 実際のcommandListのImGuiの描画コマンドを積む
  // ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
  //                              dx12Core->GetCommandList());

  // ImGuiの終了処理
  // ImGui_ImplDX12_Shutdown();
  // ImGui_ImplWin32_Shutdown();
  // ImGui::DestroyContext();

  delete game;

  return 0;
}
