#include "Matrix4x4.h"
#include "Transform.h"
#include "Vector4.h"
#include "VertexData.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <Windows.h>
#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <format>
#include <string>
#define _USE_MATH_DEFINES
#include "DirectionalLight.h"
#include "Material.h"
#include "TransformationMatrix.h"
#include <math.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  // メッセージに応じてゲーム固有の処理を行う

  if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
    return true;
  }

  switch (msg) {
    // ウィンドウが破壊された

  case WM_DESTROY:
    // osに対して、アプリの終了を伝える
    PostQuitMessage(0);
    return 0;
  }

  // 標準のメッセージ処理を行う
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

// 出力ウィンドウに文字を出す
void Log(const std::string &message) { OutputDebugStringA(message.c_str()); }

// std::wstringからstd::stringに変換する関数
std::wstring ConvertString(const std::string &str) {
  if (str.empty()) {
    return std::wstring();
  }

  auto sizeNeeded =
      MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]),
                          static_cast<int>(str.size()), NULL, 0);
  if (sizeNeeded == 0) {
    return std::wstring();
  }
  std::wstring result(sizeNeeded, 0);
  MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]),
                      static_cast<int>(str.size()), &result[0], sizeNeeded);
  return result;
}

std::string ConvertString(const std::wstring &str) {
  if (str.empty()) {
    return std::string();
  }

  auto sizeNeeded =
      WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
                          NULL, 0, NULL, NULL);
  if (sizeNeeded == 0) {
    return std::string();
  }
  std::string result(sizeNeeded, 0);
  WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
                      result.data(), sizeNeeded, NULL, NULL);
  return result;
}

IDxcBlob *CompileShader(

    // compileするshaderファイルへのパス
    const std::wstring &filePath,

    // compileに使用するprofile
    const wchar_t *profile,

    // 初期化で作成した物を三つ
    IDxcUtils *dxcUtils, IDxcCompiler3 *dxcCompiler,
    IDxcIncludeHandler *includeHandler) {

  //======================================
  // 1.hlslファイルを読む
  //======================================

  // これからシェーダーをコンパイルする旨をログに出す
  Log(ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{} \n",
                                filePath, profile)));

  // hlslファイルを読む
  IDxcBlobEncoding *shaderSource = nullptr;
  HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);

  // 読めなかったら止める
  assert(SUCCEEDED(hr));

  // 読み込んだファイルの内容を設定する
  DxcBuffer shaderSourceBuffer;
  shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
  shaderSourceBuffer.Size = shaderSource->GetBufferSize();
  shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF8の文字コードであることを通知

  //======================================
  // 2.Compileする
  //======================================

  LPCWSTR arguments[] = {

      filePath.c_str(), // コンパイル対象のhlslファイル名
      L"-E",
      L"main", // エントリーポイントの指定。基本的にmain以外にはしない
      L"-T",
      profile, // shderProfileの設定
      L"-Zi",
      L"-Qembed_debug", // デバッグ用の情報を埋め込む
      L"-Od",           // 最適化を外しておく
      L"-Zpr",          // メモリレイアウトは最優先
  };

  // 実際にshaderをコンパイルする
  IDxcResult *shaderResult = nullptr;
  hr = dxcCompiler->Compile(&shaderSourceBuffer, // 読み込んだファイル
                            arguments, // コンパイルオプション
                            _countof(arguments), // コンパイルオプションの数
                            includeHandler, // includeが含まれた諸々
                            IID_PPV_ARGS(&shaderResult));

  // コンパイルエラーではなくdxcが起動できないなど致命的な状況
  assert(SUCCEEDED(hr));

  //======================================
  // 3.警告、エラーが出ていないか確認する
  //======================================

  // 警告、エラーが出てたらログにだして止める
  IDxcBlobUtf8 *shaderError = nullptr;
  shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);

  if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
    Log(shaderError->GetStringPointer());

    assert(false);
  }

  //======================================
  // 4.Compile結果を受け取って渡す
  //======================================

  // コンパイル結果から実行用のバイナリ部分を取得
  IDxcBlob *shaderBlob = nullptr;
  hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob),
                               nullptr);

  assert(SUCCEEDED(hr));

  // 成功したログを出す
  Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n",
                                filePath, profile)));

  // もう使わないリソースを解放
  shaderSource->Release();
  shaderResult->Release();

  // 実行用のバイナリを返却
  return shaderBlob;
}

ID3D12Resource *CreateBufferResource(ID3D12Device *device, size_t sizeInBytes) {

  D3D12_HEAP_PROPERTIES uploadHeapProperties{};
  uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC bufferResourceDesc{};

  bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  bufferResourceDesc.Width = sizeInBytes;

  bufferResourceDesc.Height = 1;
  bufferResourceDesc.DepthOrArraySize = 1;
  bufferResourceDesc.MipLevels = 1;
  bufferResourceDesc.SampleDesc.Count = 1;

  bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ID3D12Resource *bufferResource = nullptr;

  HRESULT hr = device->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferResourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&bufferResource));
  assert(SUCCEEDED(hr));

  return bufferResource;
}

/// <summary>
/// 単位行列作成
/// </summary>
/// <returns>単位行列</returns>
Matrix4x4 MakeIdentity4x4() {
  Matrix4x4 matrix;

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i == j) {
        matrix.m[i][j] = 1.0f;
      } else {
        matrix.m[i][j] = 0.0f;
      }
    }
  }

  return matrix;
}

/// <summary>
/// 行列の積を求める
/// </summary>
/// <param name="matrix1">行列</param>
/// <param name="matrix2">行列</param>
/// <returns>積</returns>
Matrix4x4 Multiply(Matrix4x4 matrix1, Matrix4x4 matrix2) {

  Matrix4x4 result;

  result.m[0][0] =
      matrix1.m[0][0] * matrix2.m[0][0] + matrix1.m[0][1] * matrix2.m[1][0] +
      matrix1.m[0][2] * matrix2.m[2][0] + matrix1.m[0][3] * matrix2.m[3][0];

  result.m[0][1] =
      matrix1.m[0][0] * matrix2.m[0][1] + matrix1.m[0][1] * matrix2.m[1][1] +
      matrix1.m[0][2] * matrix2.m[2][1] + matrix1.m[0][3] * matrix2.m[3][1];

  result.m[0][2] =
      matrix1.m[0][0] * matrix2.m[0][2] + matrix1.m[0][1] * matrix2.m[1][2] +
      matrix1.m[0][2] * matrix2.m[2][2] + matrix1.m[0][3] * matrix2.m[3][2];

  result.m[0][3] =
      matrix1.m[0][0] * matrix2.m[0][3] + matrix1.m[0][1] * matrix2.m[1][3] +
      matrix1.m[0][2] * matrix2.m[2][3] + matrix1.m[3][0] * matrix2.m[3][3];

  result.m[1][0] =
      matrix1.m[1][0] * matrix2.m[0][0] + matrix1.m[1][1] * matrix2.m[1][0] +
      matrix1.m[1][2] * matrix2.m[2][0] + matrix1.m[1][3] * matrix2.m[3][0];

  result.m[1][1] =
      matrix1.m[1][0] * matrix2.m[0][1] + matrix1.m[1][1] * matrix2.m[1][1] +
      matrix1.m[1][2] * matrix2.m[2][1] + matrix1.m[1][3] * matrix2.m[3][1];

  result.m[1][2] =
      matrix1.m[1][0] * matrix2.m[0][2] + matrix1.m[1][1] * matrix2.m[1][2] +
      matrix1.m[1][2] * matrix2.m[2][2] + matrix1.m[1][3] * matrix2.m[3][2];

  result.m[1][3] =
      matrix1.m[1][0] * matrix2.m[0][3] + matrix1.m[1][1] * matrix2.m[1][3] +
      matrix1.m[1][2] * matrix2.m[2][3] + matrix1.m[1][3] * matrix2.m[3][3];

  result.m[2][0] =
      matrix1.m[2][0] * matrix2.m[0][0] + matrix1.m[2][1] * matrix2.m[1][0] +
      matrix1.m[2][2] * matrix2.m[2][0] + matrix1.m[2][3] * matrix2.m[3][0];

  result.m[2][1] =
      matrix1.m[2][0] * matrix2.m[0][1] + matrix1.m[2][1] * matrix2.m[1][1] +
      matrix1.m[2][2] * matrix2.m[2][1] + matrix1.m[2][3] * matrix2.m[3][1];

  result.m[2][2] =
      matrix1.m[2][0] * matrix2.m[0][2] + matrix1.m[2][1] * matrix2.m[1][2] +
      matrix1.m[2][2] * matrix2.m[2][2] + matrix1.m[2][3] * matrix2.m[3][2];

  result.m[2][3] =
      matrix1.m[2][0] * matrix2.m[0][3] + matrix1.m[2][1] * matrix2.m[1][3] +
      matrix1.m[2][2] * matrix2.m[2][3] + matrix1.m[2][3] * matrix2.m[3][3];

  result.m[3][0] =
      matrix1.m[3][0] * matrix2.m[0][0] + matrix1.m[3][1] * matrix2.m[1][0] +
      matrix1.m[3][2] * matrix2.m[2][0] + matrix1.m[3][3] * matrix2.m[3][0];

  result.m[3][1] =
      matrix1.m[3][0] * matrix2.m[0][1] + matrix1.m[3][1] * matrix2.m[1][1] +
      matrix1.m[3][2] * matrix2.m[2][1] + matrix1.m[3][3] * matrix2.m[3][1];

  result.m[3][2] =
      matrix1.m[3][0] * matrix2.m[0][2] + matrix1.m[3][1] * matrix2.m[1][2] +
      matrix1.m[3][2] * matrix2.m[2][2] + matrix1.m[3][3] * matrix2.m[3][2];

  result.m[3][3] =
      matrix1.m[3][0] * matrix2.m[0][3] + matrix1.m[3][1] * matrix2.m[1][3] +
      matrix1.m[3][2] * matrix2.m[2][3] + matrix1.m[3][3] * matrix2.m[3][3];

  return result;
}

// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3 &translate) {

  Matrix4x4 result;

  result.m[0][0] = 1.0f;
  result.m[0][1] = 0.0f;
  result.m[0][2] = 0.0f;
  result.m[0][3] = 0.0f;
  result.m[1][0] = 0.0f;
  result.m[1][1] = 1.0f;
  result.m[1][2] = 0.0f;
  result.m[1][3] = 0.0f;
  result.m[2][0] = 0.0f;
  result.m[2][1] = 0.0f;
  result.m[2][2] = 1.0f;
  result.m[2][3] = 0.0f;
  result.m[3][0] = translate.x;
  result.m[3][1] = translate.y;
  result.m[3][2] = translate.z;
  result.m[3][3] = 1.0f;

  return result;
}

// 拡縮行列
Matrix4x4 MakeScaleMatrix(const Vector3 &scale) {

  Matrix4x4 result;

  result.m[0][0] = scale.x;
  result.m[0][1] = 0.0f;
  result.m[0][2] = 0.0f;
  result.m[0][3] = 0.0f;
  result.m[1][0] = 0.0f;
  result.m[1][1] = scale.y;
  result.m[1][2] = 0.0f;
  result.m[1][3] = 0.0f;
  result.m[2][0] = 0.0f;
  result.m[2][1] = 0.0f;
  result.m[2][2] = scale.z;
  result.m[2][3] = 0.0f;
  result.m[3][0] = 0.0f;
  result.m[3][1] = 0.0f;
  result.m[3][2] = 0.0f;
  result.m[3][3] = 1.0f;

  return result;
}

Matrix4x4 MakeRotateZMatrix(const float &rotateZ) {

  Matrix4x4 result;
  result.m[0][0] = cosf(rotateZ);
  result.m[0][1] = sinf(rotateZ);
  result.m[0][2] = 0.0f;
  result.m[0][3] = 0.0f;
  result.m[1][0] = -sinf(rotateZ);
  result.m[1][1] = cosf(rotateZ);
  result.m[1][2] = 0.0f;
  result.m[1][3] = 0.0f;
  result.m[2][0] = 0.0f;
  result.m[2][1] = 0.0f;
  result.m[2][2] = 1.0f;
  result.m[2][3] = 0.0f;
  result.m[3][0] = 0.0f;
  result.m[3][1] = 0.0f;
  result.m[3][2] = 0.0f;
  result.m[3][3] = 1.0f;

  return result;
}

Matrix4x4 MakeRotateMatrix(const Vector3 &rotate) {
  Matrix4x4 rotateX;
  rotateX.m[0][0] = 1.0f;
  rotateX.m[0][1] = 0.0f;
  rotateX.m[0][2] = 0.0f;
  rotateX.m[0][3] = 0.0f;
  rotateX.m[1][0] = 0.0f;
  rotateX.m[1][1] = cosf(rotate.x);
  rotateX.m[1][2] = sinf(rotate.x);
  rotateX.m[1][3] = 0.0f;
  rotateX.m[2][0] = 0.0f;
  rotateX.m[2][1] = -sinf(rotate.x);
  rotateX.m[2][2] = cosf(rotate.x);
  rotateX.m[2][3] = 0.0f;
  rotateX.m[3][0] = 0.0f;
  rotateX.m[3][1] = 0.0f;
  rotateX.m[3][2] = 0.0f;
  rotateX.m[3][3] = 1.0f;

  Matrix4x4 rotateY;
  rotateY.m[0][0] = cosf(rotate.y);
  rotateY.m[0][1] = 0.0f;
  rotateY.m[0][2] = -sinf(rotate.y);
  rotateY.m[0][3] = 0.0f;
  rotateY.m[1][0] = 0.0f;
  rotateY.m[1][1] = 1.0f;
  rotateY.m[1][2] = 0.0f;
  rotateY.m[1][3] = 0.0f;
  rotateY.m[2][0] = sinf(rotate.y);
  rotateY.m[2][1] = 0.0f;
  rotateY.m[2][2] = cosf(rotate.y);
  rotateY.m[2][3] = 0.0f;
  rotateY.m[3][0] = 0.0f;
  rotateY.m[3][1] = 0.0f;
  rotateY.m[3][2] = 0.0f;
  rotateY.m[3][3] = 1.0f;

  Matrix4x4 rotateZ;
  rotateZ.m[0][0] = cosf(rotate.z);
  rotateZ.m[0][1] = sinf(rotate.z);
  rotateZ.m[0][2] = 0.0f;
  rotateZ.m[0][3] = 0.0f;
  rotateZ.m[1][0] = -sinf(rotate.z);
  rotateZ.m[1][1] = cosf(rotate.z);
  rotateZ.m[1][2] = 0.0f;
  rotateZ.m[1][3] = 0.0f;
  rotateZ.m[2][0] = 0.0f;
  rotateZ.m[2][1] = 0.0f;
  rotateZ.m[2][2] = 1.0f;
  rotateZ.m[2][3] = 0.0f;
  rotateZ.m[3][0] = 0.0f;
  rotateZ.m[3][1] = 0.0f;
  rotateZ.m[3][2] = 0.0f;
  rotateZ.m[3][3] = 1.0f;

  Matrix4x4 result = Multiply(rotateX, Multiply(rotateY, rotateZ));

  return result;
}

/// <summary>
/// アフィン行列作成
/// </summary>
/// <param name="scale">拡縮</param>
/// <param name="rotate">回転</param>
/// <param name="translate">平行移動</param>
/// <returns>アフィン行列</returns>
Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate) {
  Matrix4x4 scaleMatrix;

  scaleMatrix.m[0][0] = scale.x;
  scaleMatrix.m[0][1] = 0.0f;
  scaleMatrix.m[0][2] = 0.0f;
  scaleMatrix.m[0][3] = 0.0f;
  scaleMatrix.m[1][0] = 0.0f;
  scaleMatrix.m[1][1] = scale.y;
  scaleMatrix.m[1][2] = 0.0f;
  scaleMatrix.m[1][3] = 0.0f;
  scaleMatrix.m[2][0] = 0.0f;
  scaleMatrix.m[2][1] = 0.0f;
  scaleMatrix.m[2][2] = scale.z;
  scaleMatrix.m[2][3] = 0.0f;
  scaleMatrix.m[3][0] = 0.0f;
  scaleMatrix.m[3][1] = 0.0f;
  scaleMatrix.m[3][2] = 0.0f;
  scaleMatrix.m[3][3] = 1.0f;

  Matrix4x4 rotateX;
  rotateX.m[0][0] = 1.0f;
  rotateX.m[0][1] = 0.0f;
  rotateX.m[0][2] = 0.0f;
  rotateX.m[0][3] = 0.0f;
  rotateX.m[1][0] = 0.0f;
  rotateX.m[1][1] = cosf(rotate.x);
  rotateX.m[1][2] = sinf(rotate.x);
  rotateX.m[1][3] = 0.0f;
  rotateX.m[2][0] = 0.0f;
  rotateX.m[2][1] = -sinf(rotate.x);
  rotateX.m[2][2] = cosf(rotate.x);
  rotateX.m[2][3] = 0.0f;
  rotateX.m[3][0] = 0.0f;
  rotateX.m[3][1] = 0.0f;
  rotateX.m[3][2] = 0.0f;
  rotateX.m[3][3] = 1.0f;

  Matrix4x4 rotateY;
  rotateY.m[0][0] = cosf(rotate.y);
  rotateY.m[0][1] = 0.0f;
  rotateY.m[0][2] = -sinf(rotate.y);
  rotateY.m[0][3] = 0.0f;
  rotateY.m[1][0] = 0.0f;
  rotateY.m[1][1] = 1.0f;
  rotateY.m[1][2] = 0.0f;
  rotateY.m[1][3] = 0.0f;
  rotateY.m[2][0] = sinf(rotate.y);
  rotateY.m[2][1] = 0.0f;
  rotateY.m[2][2] = cosf(rotate.y);
  rotateY.m[2][3] = 0.0f;
  rotateY.m[3][0] = 0.0f;
  rotateY.m[3][1] = 0.0f;
  rotateY.m[3][2] = 0.0f;
  rotateY.m[3][3] = 1.0f;

  Matrix4x4 rotateZ;
  rotateZ.m[0][0] = cosf(rotate.z);
  rotateZ.m[0][1] = sinf(rotate.z);
  rotateZ.m[0][2] = 0.0f;
  rotateZ.m[0][3] = 0.0f;
  rotateZ.m[1][0] = -sinf(rotate.z);
  rotateZ.m[1][1] = cosf(rotate.z);
  rotateZ.m[1][2] = 0.0f;
  rotateZ.m[1][3] = 0.0f;
  rotateZ.m[2][0] = 0.0f;
  rotateZ.m[2][1] = 0.0f;
  rotateZ.m[2][2] = 1.0f;
  rotateZ.m[2][3] = 0.0f;
  rotateZ.m[3][0] = 0.0f;
  rotateZ.m[3][1] = 0.0f;
  rotateZ.m[3][2] = 0.0f;
  rotateZ.m[3][3] = 1.0f;

  Matrix4x4 rotateMatrix = Multiply(rotateX, Multiply(rotateY, rotateZ));

  Matrix4x4 translateMatrix;
  translateMatrix.m[0][0] = 1.0f;
  translateMatrix.m[0][1] = 0.0f;
  translateMatrix.m[0][2] = 0.0f;
  translateMatrix.m[0][3] = 0.0f;
  translateMatrix.m[1][0] = 0.0f;
  translateMatrix.m[1][1] = 1.0f;
  translateMatrix.m[1][2] = 0.0f;
  translateMatrix.m[1][3] = 0.0f;
  translateMatrix.m[2][0] = 0.0f;
  translateMatrix.m[2][1] = 0.0f;
  translateMatrix.m[2][2] = 1.0f;
  translateMatrix.m[2][3] = 0.0f;
  translateMatrix.m[3][0] = translate.x;
  translateMatrix.m[3][1] = translate.y;
  translateMatrix.m[3][2] = translate.z;
  translateMatrix.m[3][3] = 1.0f;

  Matrix4x4 worldMatrix;
  worldMatrix = Multiply(scaleMatrix, Multiply(rotateMatrix, translateMatrix));

  return worldMatrix;
}

/// <summary>
/// 透視投影行列作成
/// </summary>
/// <param name="fovY">画角</param>
/// <param name="aspectRatio">アスペクト比</param>
/// <param name="nearClip">近平面</param>
/// <param name="farClip">遠平面</param>
/// <returns>透視投影行列</returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip) {
  Matrix4x4 matrix;

  matrix.m[0][0] = 1.0f / aspectRatio * (cosf(fovY / 2.0f) / sinf(fovY / 2.0f));
  matrix.m[0][1] = 0.0f;
  matrix.m[0][2] = 0.0f;
  matrix.m[0][3] = 0.0f;
  matrix.m[1][0] = 0.0f;
  matrix.m[1][1] = cosf(fovY / 2.0f) / sinf(fovY / 2.0f);
  matrix.m[1][2] = 0.0f;
  matrix.m[1][3] = 0.0f;
  matrix.m[2][0] = 0.0f;
  matrix.m[2][1] = 0.0f;
  matrix.m[2][2] = farClip / (farClip - nearClip);
  matrix.m[2][3] = 1.0f;
  matrix.m[3][0] = 0.0f;
  matrix.m[3][1] = 0.0f;
  matrix.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);
  matrix.m[3][3] = 0.0f;

  return matrix;
}

/// <summary>
/// 正射影行列(平行投影行列)作成
/// </summary>
/// <param name="left">左端</param>
/// <param name="top">上端</param>
/// <param name="right">右端</param>
/// <param name="bottom">下端</param>
/// <param name="nearClip">近平面</param>
/// <param name="farClip">遠平面</param>
/// <returns>正射影行列</returns>
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip) {
  Matrix4x4 matrix;

  matrix.m[0][0] = 2.0f / (right - left);
  matrix.m[0][1] = 0.0f;
  matrix.m[0][2] = 0.0f;
  matrix.m[0][3] = 0.0f;
  matrix.m[1][0] = 0.0f;
  matrix.m[1][1] = 2.0f / (top - bottom);
  matrix.m[1][2] = 0.0f;
  matrix.m[1][3] = 0.0f;
  matrix.m[2][0] = 0.0f;
  matrix.m[2][1] = 0.0f;
  matrix.m[2][2] = 1.0f / (farClip - nearClip);
  matrix.m[2][3] = 0.0f;
  matrix.m[3][0] = (left + right) / (left - right);
  matrix.m[3][1] = (top + bottom) / (bottom - top);
  matrix.m[3][2] = nearClip / (nearClip - farClip);
  matrix.m[3][3] = 1.0f;

  return matrix;
}

/// <summary>
/// 4x4逆行列生成
/// </summary>
/// <param name="matrix">4x4正則行列</param>
/// <returns>逆行列</returns>
Matrix4x4 Inverse(Matrix4x4 matrix) {
  Matrix4x4 inverseMatrix;

  float det =
      matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][3] +
      matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][1] +
      matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][2] -
      matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][1] -
      matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][3] -
      matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][2] -
      matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][3] -
      matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][1] -
      matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][2] +
      matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][1] +
      matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][3] +
      matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][2] +
      matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][3] +
      matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][1] +
      matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][2] -
      matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][1] -
      matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][3] -
      matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][2] -
      matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][0] -
      matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][0] -
      matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][0] +
      matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][0] +
      matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][0] +
      matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][0];

  float invDet = 1 / det;

  // 逆行列を求めるための補助行列を計算
  inverseMatrix.m[0][0] = (matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][3] -
                           matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][2] -
                           matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][3] +
                           matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][1] +
                           matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][2] -
                           matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][1]) *
                          invDet;

  inverseMatrix.m[0][1] = (-(matrix.m[0][1] * matrix.m[2][2] * matrix.m[3][3]) +
                           matrix.m[0][1] * matrix.m[2][3] * matrix.m[3][2] +
                           matrix.m[0][2] * matrix.m[2][1] * matrix.m[3][3] -
                           matrix.m[0][2] * matrix.m[2][3] * matrix.m[3][1] -
                           matrix.m[0][3] * matrix.m[2][1] * matrix.m[3][2] +
                           matrix.m[0][3] * matrix.m[2][2] * matrix.m[3][1]) *
                          invDet;

  inverseMatrix.m[0][2] = (matrix.m[0][1] * matrix.m[1][2] * matrix.m[3][3] -
                           matrix.m[0][1] * matrix.m[1][3] * matrix.m[3][2] -
                           matrix.m[0][2] * matrix.m[1][1] * matrix.m[3][3] +
                           matrix.m[0][2] * matrix.m[1][3] * matrix.m[3][1] +
                           matrix.m[0][3] * matrix.m[1][1] * matrix.m[3][2] -
                           matrix.m[0][3] * matrix.m[1][2] * matrix.m[3][1]) *
                          invDet;

  inverseMatrix.m[0][3] = (-(matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][3]) +
                           matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][2] +
                           matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][3] -
                           matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][1] -
                           matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][2] +
                           matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][1]) *
                          invDet;
  inverseMatrix.m[1][0] = (-(matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][3]) +
                           matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][2] +
                           matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][3] -
                           matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][0] -
                           matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][2] +
                           matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[1][1] = (matrix.m[0][0] * matrix.m[2][2] * matrix.m[3][3] -
                           matrix.m[0][0] * matrix.m[2][3] * matrix.m[3][2] -
                           matrix.m[0][2] * matrix.m[2][0] * matrix.m[3][3] +
                           matrix.m[0][2] * matrix.m[2][3] * matrix.m[3][0] +
                           matrix.m[0][3] * matrix.m[2][0] * matrix.m[3][2] -
                           matrix.m[0][3] * matrix.m[2][2] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[1][2] = (-(matrix.m[0][0] * matrix.m[1][2] * matrix.m[3][3]) +
                           matrix.m[0][0] * matrix.m[1][3] * matrix.m[3][2] +
                           matrix.m[0][2] * matrix.m[1][0] * matrix.m[3][3] -
                           matrix.m[0][2] * matrix.m[1][3] * matrix.m[3][0] -
                           matrix.m[0][3] * matrix.m[1][0] * matrix.m[3][2] +
                           matrix.m[0][3] * matrix.m[1][2] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[1][3] = (matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][3] -
                           matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][2] -
                           matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][3] +
                           matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][0] +
                           matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][2] -
                           matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][0]) *
                          invDet;

  inverseMatrix.m[2][0] = (matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][3] -
                           matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][1] -
                           matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][3] +
                           matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][0] +
                           matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][1] -
                           matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[2][1] = (-(matrix.m[0][0] * matrix.m[2][1] * matrix.m[3][3]) +
                           matrix.m[0][0] * matrix.m[2][3] * matrix.m[3][1] +
                           matrix.m[0][1] * matrix.m[2][0] * matrix.m[3][3] -
                           matrix.m[0][1] * matrix.m[2][3] * matrix.m[3][0] -
                           matrix.m[0][3] * matrix.m[2][0] * matrix.m[3][1] +
                           matrix.m[0][3] * matrix.m[2][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[2][2] = (matrix.m[0][0] * matrix.m[1][1] * matrix.m[3][3] -
                           matrix.m[0][0] * matrix.m[1][3] * matrix.m[3][1] -
                           matrix.m[0][1] * matrix.m[1][0] * matrix.m[3][3] +
                           matrix.m[0][1] * matrix.m[1][3] * matrix.m[3][0] +
                           matrix.m[0][3] * matrix.m[1][0] * matrix.m[3][1] -
                           matrix.m[0][3] * matrix.m[1][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[2][3] = (-(matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][3]) +
                           matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][1] +
                           matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][3] -
                           matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][0] -
                           matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][1] +
                           matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][0]) *
                          invDet;

  inverseMatrix.m[3][0] = (-(matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][2]) +
                           matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][1] +
                           matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][2] -
                           matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][0] -
                           matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][1] +
                           matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[3][1] = (matrix.m[0][0] * matrix.m[2][1] * matrix.m[3][2] -
                           matrix.m[0][0] * matrix.m[2][2] * matrix.m[3][1] -
                           matrix.m[0][1] * matrix.m[2][0] * matrix.m[3][2] +
                           matrix.m[0][1] * matrix.m[2][2] * matrix.m[3][0] +
                           matrix.m[0][2] * matrix.m[2][0] * matrix.m[3][1] -
                           matrix.m[0][2] * matrix.m[2][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[3][2] = (-(matrix.m[0][0] * matrix.m[1][1] * matrix.m[3][2]) +
                           matrix.m[0][0] * matrix.m[1][2] * matrix.m[3][1] +
                           matrix.m[0][1] * matrix.m[1][0] * matrix.m[3][2] -
                           matrix.m[0][1] * matrix.m[1][2] * matrix.m[3][0] -
                           matrix.m[0][2] * matrix.m[1][0] * matrix.m[3][1] +
                           matrix.m[0][2] * matrix.m[1][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[3][3] = (matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] -
                           matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1] -
                           matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] +
                           matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] +
                           matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1] -
                           matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0]) *
                          invDet;

  return inverseMatrix;
}

ID3D12DescriptorHeap *CreateDescriptorHeap(ID3D12Device *device,
                                           D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                           UINT numDescriptors,
                                           bool shaderVisible) {
  // ディスクリプタヒープの生成
  ID3D12DescriptorHeap *descriptorHeap = nullptr;
  D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
  descriptorHeapDesc.Type = heapType;
  descriptorHeapDesc.NumDescriptors = numDescriptors;
  descriptorHeapDesc.Flags = shaderVisible
                                 ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                 : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc,
                                            IID_PPV_ARGS(&descriptorHeap));
  assert(SUCCEEDED(hr));

  return descriptorHeap;
}

DirectX::ScratchImage LoadTexture(const std::string &filePath) {

  // テクスチャファイルを読んでプログラムで扱えるようにする
  DirectX::ScratchImage image{};
  std::wstring filePathW = ConvertString(filePath);
  HRESULT hr = DirectX::LoadFromWICFile(
      filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
  assert(SUCCEEDED(hr));

  // ミニマップの作成
  DirectX::ScratchImage mipImages{};
  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                0, mipImages);
  assert(SUCCEEDED(hr));

  // ミニマップ付きのデータを返す
  return mipImages;
}

/// <summary>
/// TextureResourceを作る
/// </summary>
/// <param name="device"></param>
/// <param name="metadata"></param>
/// <returns></returns>
ID3D12Resource *CreateTextureResource(ID3D12Device *device,
                                      const DirectX::TexMetadata &metadata) {

  // 1.metadataを基にResourceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = UINT(metadata.width);           // Textureの幅
  resourceDesc.Height = UINT(metadata.height);         // Textureの高さ
  resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
  resourceDesc.DepthOrArraySize =
      UINT16(metadata.arraySize);        // 奥行 or 配列Textureの配列数
  resourceDesc.Format = metadata.format; // TextureのFormat
  resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
  resourceDesc.Dimension =
      D3D12_RESOURCE_DIMENSION(metadata.dimension); // textureの次元数

  // 利用するHeapの設定。
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
  heapProperties.CPUPageProperty =
      D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // writeBackポリシーでCPUアクセス可能
  heapProperties.MemoryPoolPreference =
      D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置

  // Resourceの生成
  ID3D12Resource *resource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties,      // Heapの設定
      D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定
      &resourceDesc,        // Resourceの設定
      D3D12_RESOURCE_STATE_GENERIC_READ, // 初回のResourceState。Textureは基本読むだけ
      nullptr,                  // Clear最適値。使用しないのでnullptr
      IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ

  assert(SUCCEEDED(hr));
  return resource;
}

void UploadTextureData(ID3D12Resource *texture,
                       const DirectX::ScratchImage &mipImages) {

  // Meta情報を取得
  const DirectX::TexMetadata &metaData = mipImages.GetMetadata();

  for (size_t mipLevel = 0; mipLevel < metaData.mipLevels; ++mipLevel) {

    // MipMapLevelを指定して各Imageを取得
    const DirectX::Image *img = mipImages.GetImage(mipLevel, 0, 0);

    // Textureに転送
    HRESULT hr =
        texture->WriteToSubresource(UINT(mipLevel),
                                    nullptr,     // 全領域へコピー
                                    img->pixels, // 元データアドレス
                                    UINT(img->rowPitch),  // 1ラインサイズ
                                    UINT(img->slicePitch) // 1枚サイズ
        );

    assert(SUCCEEDED(hr));
  }
}

ID3D12Resource *CreateDepthStencilTextureResource(ID3D12Device *device,
                                                  int32_t width,
                                                  int32_t height) {

  // 生成するResourceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = width;        // Textureの幅
  resourceDesc.Height = height;      // 高さ
  resourceDesc.MipLevels = 1;        // mipmapの数
  resourceDesc.DepthOrArraySize = 1; // 奥行き or 配列Textureの配列数
  resourceDesc.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとして利用可能なフォーマット
  resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 二次元
  resourceDesc.Flags =
      D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う

  // 利用するheapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

  // 深度値のクリア設定
  D3D12_CLEAR_VALUE depthClearValue{};
  depthClearValue.DepthStencil.Depth = 1.0f; // 最大値(1.0f)でクリア
  depthClearValue.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる

  // Resourceの生成
  ID3D12Resource *resource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties,                  // heapの設定
      D3D12_HEAP_FLAG_NONE,             // heapの特殊な設定
      &resourceDesc,                    // Resourceの設定
      D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
      &depthClearValue,                 // Clear最適値
      IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ

  assert(SUCCEEDED(hr));

  return resource;
}

/// <summary>
/// CPUHandleの取得
/// </summary>
/// <param name="descriptorHeap">DescriptorHandle</param>
/// <param name="descriptorSize">DescriptorSize</param>
/// <param name="index">index</param>
/// <returns></returns>
D3D12_CPU_DESCRIPTOR_HANDLE
GetCPUDescriptorHandle(ID3D12DescriptorHeap *descriptorHeap,
                       uint32_t descriptorSize, uint32_t index) {

  D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
      descriptorHeap->GetCPUDescriptorHandleForHeapStart();
  handleCPU.ptr += (descriptorSize * index);
  return handleCPU;
}

/// <summary>
/// GPUHandleの取得
/// </summary>
/// <param name="descriptorHeap">descriptorHeap</param>
/// <param name="descriptorSize">descriptorSize</param>
/// <param name="index">index</param>
/// <returns></returns>
D3D12_GPU_DESCRIPTOR_HANDLE
GetGPUDescriptorHandle(ID3D12DescriptorHeap *descriptorHeap,
                       uint32_t descriptorSize, uint32_t index) {

  D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
      descriptorHeap->GetGPUDescriptorHandleForHeapStart();
  handleGPU.ptr += (descriptorSize * index);
  return handleGPU;
}

/// <summary>
/// Vector3 正規化
/// </summary>
/// <returns>正規化された値</returns>
Vector3 Normalize(Vector3 vector) {

  // 長さ
  float length;

  length =
      sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);

  return {vector.x / length, vector.y / length, vector.z / length};
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  CoInitializeEx(0, COINIT_MULTITHREADED);

  WNDCLASS wc{};

  // ウィンドウプロシージャ
  wc.lpfnWndProc = WindowProc;

  // ウィンドウクラス名
  wc.lpszClassName = L"CG2WindowClass";

  // インスタンスハンドル
  wc.hInstance = GetModuleHandle(nullptr);

  // カーソル
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

  // ウィンドウクラスを登録する
  RegisterClass(&wc);

  // 出力ウィンドウへの文字出力
  OutputDebugStringA("Hello,DirectX!\n");

  // クライアント領域のサイズ
  const int32_t kClientWidth = 1280;
  const int32_t kClientHeight = 720;

  // ウィンドウサイズを表す構造体にクライアント領域を入れる
  RECT wrc = {0, 0, kClientWidth, kClientHeight};

  // クライアント領域をもとに実際のサイズにwrcを変更してもらう
  AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

  // ウィンドウの生成
  HWND hwnd = CreateWindow(

      wc.lpszClassName,     // 利用するクラス名
      L"CG2",               // タイトルバーの文字
      WS_OVERLAPPEDWINDOW,  // よく見るウィンドウスタイル
      CW_USEDEFAULT,        // 表示X座標
      CW_USEDEFAULT,        // 表示Y座標
      wrc.right - wrc.left, // ウィンドウ横幅
      wrc.bottom - wrc.top, // ウィンドウ縦幅
      nullptr,              // 親ウィンドウハンドル
      nullptr,              // メニューハンドル
      wc.hInstance,         // インスタンスハンドル
      nullptr);             // オプション

#ifdef _DEBUG

  ID3D12Debug1 *debugController = nullptr;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {

    // デバッグレイヤーを有効化する
    debugController->EnableDebugLayer();

    // さらにGPU側でもチェックを行うようにする
    debugController->SetEnableGPUBasedValidation(TRUE);
  }

#endif

  // ウィンドウを表示する
  ShowWindow(hwnd, SW_SHOW);

  // DXGIファクトリーの生成
  IDXGIFactory7 *dxgiFactory = nullptr;

  HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

  assert(SUCCEEDED(hr));

  // 使用するアダプタ用の変数。最初にnullptrを入れておく
  IDXGIAdapter4 *useAdapter = nullptr;

  // 良い順にアダプタを頼む
  for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
                       i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                       IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
       ++i) {

    // アダプターの情報を取得する
    DXGI_ADAPTER_DESC3 adapterDesc{};
    hr = useAdapter->GetDesc3(&adapterDesc);
    assert(SUCCEEDED(hr));

    // ソフトウェアアダプタでなければ採用
    if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
      // 採用したアダプタの情報をログに出力。wstring
      Log(ConvertString(
          std::format(L"Use Adapter:{}\n,", adapterDesc.Description)));
      break;
    }

    useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
  }

  // 適切なアダプタが見つからなかったので起動できない
  assert(useAdapter != nullptr);

  ID3D12Device *device = nullptr;

  // 機能レベルとログ出力用の文字列
  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0};

  const char *featureLevelStrings[] = {"12.2", "12.1", "12.0"};

  // 高い順に生成できるか試していく
  for (size_t i = 0; i < _countof(featureLevels); ++i) {

    // 採用したアダプターデバイスを生成
    hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));

    // 指定した機能レベルでデバイスが生成できたかを確認
    if (SUCCEEDED(hr)) {
      // 生成できたのでログ出力を行ってループを抜ける
      Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
      break;
    }
  }

  // デバイスの生成がうまくいかなかったので起動できない
  assert(device != nullptr);
  Log("Complete create D3D12Device!!!\n"); // 初期化完了のログ

#ifdef _DEBUG

  ID3D12InfoQueue *infoQueue = nullptr;

  if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {

    // ヤバいエラー時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

    // エラー時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

    // 警告時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

    // 抑制するメッセージのID
    D3D12_MESSAGE_ID denyIds[] = {

        // Windows11でのDXGIデバッグレイヤーとDX12のデバッグレイヤーの相互作用バグによるエラーメッセージ
        D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE

    };

    // 抑制するレベル
    D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
    D3D12_INFO_QUEUE_FILTER filter{};
    filter.DenyList.NumIDs = _countof(denyIds);
    filter.DenyList.pIDList = denyIds;
    filter.DenyList.NumSeverities = _countof(severities);
    filter.DenyList.pSeverityList = severities;

    // 指定したメッセージの表示を抑制する
    infoQueue->PushStorageFilter(&filter);

    // 解放
    infoQueue->Release();
  }

#endif // DEBUG

  // コマンドキューを生成する
  ID3D12CommandQueue *commandQueue = nullptr;
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  hr = device->CreateCommandQueue(&commandQueueDesc,
                                  IID_PPV_ARGS(&commandQueue));

  // コマンドキューの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  // コマンドアロケータを生成する
  ID3D12CommandAllocator *commandAllocator = nullptr;
  hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      IID_PPV_ARGS(&commandAllocator));

  // コマンドアロケータの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  // コマンドリストを生成する
  ID3D12GraphicsCommandList *commandList = nullptr;
  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                 commandAllocator, nullptr,
                                 IID_PPV_ARGS(&commandList));

  // コマンドリストの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  // スワップチェーンを生成する
  IDXGISwapChain4 *swapChain = nullptr;
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
  swapChainDesc.Width =
      kClientWidth; // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
  swapChainDesc.Height =
      kClientHeight; // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
  swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
  swapChainDesc.BufferUsage =
      DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
  swapChainDesc.BufferCount = 2; // ダブルバッファ
  swapChainDesc.SwapEffect =
      DXGI_SWAP_EFFECT_FLIP_DISCARD; // モニタにうつしたら、中身を破壊

  // コマンドキュー、ウィンドウハンドル、設定を渡して生成する
  hr = dxgiFactory->CreateSwapChainForHwnd(
      commandQueue, hwnd, &swapChainDesc, nullptr, nullptr,
      reinterpret_cast<IDXGISwapChain1 **>(&swapChain));
  assert(SUCCEEDED(hr));
  // DepthStencilTextureをウィンドウサイズで作成
  ID3D12Resource *depthStencilResource =
      CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);

  //===========================
  // ディスクリプタヒープの生成
  //===========================

  // DescriptorSizeの取得
  const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  const uint32_t descriptorSizeRTV =
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  const uint32_t descriptorSizeDSV =
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

  // RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisibleはfalse
  ID3D12DescriptorHeap *rtvDescriptorHeap =
      CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

  // SRV用のヒープでディスクリプタの数は128。STVはShader内で触るものなので、ShaderVisibleはtrue
  ID3D12DescriptorHeap *srvDescriptorHeap = CreateDescriptorHeap(
      device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

  // DSV用のヒープのでディスクリプタの数は1。DSVはShader内で触るものではないので、shaderVisibleはfalse
  ID3D12DescriptorHeap *dsvDescriptorHeap =
      CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

  // DSVの設定
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
  dsvDesc.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // format。基本的にはResourceに合わせる
  dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2dTexture

  // DSVHeapの先頭にDSVを作る
  device->CreateDepthStencilView(
      depthStencilResource, &dsvDesc,
      dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

  // Textureを読んで転送
  DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
  const DirectX::TexMetadata &metadata = mipImages.GetMetadata();
  ID3D12Resource *textureResource = CreateTextureResource(device, metadata);
  UploadTextureData(textureResource, mipImages);

  // 二枚目のTextureを読んで転送
  DirectX::ScratchImage mipImages2 = LoadTexture("resources/monsterBall.png");
  const DirectX::TexMetadata &metadata2 = mipImages2.GetMetadata();
  ID3D12Resource *textureResource2 = CreateTextureResource(device, metadata2);
  UploadTextureData(textureResource2, mipImages2);

  // metadataを基にSRVの設定
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
  srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
  srvDesc2.Format = metadata2.format;
  srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
  srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

  // SRVを作成するDescriptorHeapの場所を決める
  D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU =
      GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU =
      GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);

  D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 =
      GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 =
      GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);

  // SRVの生成
  device->CreateShaderResourceView(textureResource, &srvDesc,
                                   textureSrvHandleCPU);

  device->CreateShaderResourceView(textureResource2, &srvDesc2,
                                   textureSrvHandleCPU2);

  // ディスクリプタヒープが作れなかったので起動できない
  assert(SUCCEEDED(hr));

  // SwapChainからResourceを引っ張ってくる
  ID3D12Resource *swapChainResources[2] = {nullptr};

  hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));

  // うまく取得できなければ起動できない
  assert(SUCCEEDED(hr));

  hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));

  assert(SUCCEEDED(hr));

  // RTVの設定
  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
  rtvDesc.Format =
      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
  rtvDesc.ViewDimension =
      D3D12_RTV_DIMENSION_TEXTURE2D; // 2dテクスチャとして書き込む

  // ディスクリプタの先頭を取得する
  D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle =
      GetCPUDescriptorHandle(rtvDescriptorHeap, descriptorSizeRTV, 0);

  // RTVを二つ作るのでディスクリプタを二つ用意
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

  // まず一つ目を作る。一つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある
  rtvHandles[0] = rtvStartHandle;
  device->CreateRenderTargetView(swapChainResources[0], &rtvDesc,
                                 rtvHandles[0]);

  // 二つ目のディスクリプタハンドルを得る
  rtvHandles[1].ptr =
      rtvHandles[0].ptr +
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  // 二つ目を作る
  device->CreateRenderTargetView(swapChainResources[1], &rtvDesc,
                                 rtvHandles[1]);

  // 初期値0でFenceを作る
  ID3D12Fence *fence = nullptr;
  uint64_t fenceValue = 0;
  hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(&fence));
  assert(SUCCEEDED(hr));

  // FenceのSignalを待つためのイベントを作成
  HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  assert(fenceEvent != nullptr);

  // dxCompilerを初期化
  IDxcUtils *dxcUtils = nullptr;
  IDxcCompiler3 *dxcCompiler = nullptr;
  hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
  assert(SUCCEEDED(hr));

  hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
  assert(SUCCEEDED(hr));

  // 現時点でincludeはしないが、includeに対応するための設定をしておく
  IDxcIncludeHandler *includeHandler = nullptr;
  hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
  assert(SUCCEEDED(hr));

  // RootSignatureを作成
  D3D12_ROOT_SIGNATURE_DESC descroptionRootSignature{};
  descroptionRootSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  // RootParameter作成。複数設定できるので配列。今回は結果一つだけなので長さ1の配列
  D3D12_ROOT_PARAMETER rootParameters[4] = {};
  rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
  rootParameters[0].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL;               // PixelShaderで使う
  rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド

  rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
  rootParameters[1].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_VERTEX;              // VertexShaderで使う
  rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0を使う
  descroptionRootSignature.pParameters =
      rootParameters; // ルートパラメータ配列へのポインタ
  descroptionRootSignature.NumParameters =
      _countof(rootParameters); // 配列の長さ

  D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  descriptorRange[0].BaseShaderRegister = 0; // ０から始まる
  descriptorRange[0].NumDescriptors = 1;     // 数は1つ
  descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
  descriptorRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

  rootParameters[2].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DiscriptorTableを使う
  rootParameters[2].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
  rootParameters[2].DescriptorTable.pDescriptorRanges =
      descriptorRange; // Tableの中身の配列を指定
  rootParameters[2].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRange); // Tableで利用する数

  rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
  rootParameters[3].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL;               // PSで使う
  rootParameters[3].Descriptor.ShaderRegister = 1; // レジスタ番号1を使う

  D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
  staticSamplers[0].Filter =
      D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
  staticSamplers[0].AddressU =
      D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
  staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
  staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
  staticSamplers[0].ShaderRegister = 0; // レジスタ番号0
  staticSamplers[0].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // pixelShaderを使う
  descroptionRootSignature.pStaticSamplers = staticSamplers;
  descroptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

  /*マテリアル用のリソースを作る。
  --------------------------------*/
  ID3D12Resource *materialResource =
      CreateBufferResource(device, sizeof(Material));

  // マテリアルにデータを書き込む
  Material *materialData = nullptr;

  // 書き込むためのアドレスを取得
  materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));

  assert(SUCCEEDED(hr));

  // 色の指定
  materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  // Lightingさせるか
  materialData->enableLighting = true;
  // UVTransform 単位行列を入れておく
  materialData->uvTransform = MakeIdentity4x4();

  /*Sprite用のマテリアルリソースを作る
  -----------------------------------*/
  ID3D12Resource *materialResourceSprite =
      CreateBufferResource(device, sizeof(Material));

  // データを書き込む
  Material *materialDataSprite = nullptr;

  // 書き込むためのアドレスを取得
  materialResourceSprite->Map(0, nullptr,
                              reinterpret_cast<void **>(&materialDataSprite));

  assert(SUCCEEDED(hr));

  // 色の指定
  materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  // Lightingさせるか
  materialDataSprite->enableLighting = false;
  // UVTransform　単位行列を入れておく
  materialDataSprite->uvTransform = MakeIdentity4x4();

  /*WVP用のリソースを作る。
  --------------------------------------------------*/

  // cbvのsizeは256バイト単位で確保する(GPT調べ)
  UINT transformationMatrixSize = (sizeof(TransformationMatrix) + 255) & ~255;

  assert(transformationMatrixSize % 256 == 0);

  ID3D12Resource *transformationMatrixResource =
      CreateBufferResource(device, transformationMatrixSize);

  // データを書き込む
  TransformationMatrix *transformationMatrixData = nullptr;

  // 書き込むためのアドレスを取得
  transformationMatrixResource->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData));

  // 単位行列を書き込んでおく
  transformationMatrixData->World = MakeIdentity4x4();
  transformationMatrixData->WVP = MakeIdentity4x4();

  /* Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4一つ分
  ----------------------------------------------------------------*/
  ID3D12Resource *transformationMatrixResourceSprite =
      CreateBufferResource(device, transformationMatrixSize);

  // データを書き込む
  TransformationMatrix *transformationMatrixDataSprite = nullptr;

  // 書き込むためのアドレスを取得
  transformationMatrixResourceSprite->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixDataSprite));

  // 単位行列を書き込んでおく
  transformationMatrixDataSprite->World = MakeIdentity4x4();
  transformationMatrixDataSprite->WVP = MakeIdentity4x4();

  /* 平行光源用のリソースを作る
  ------------------------------------*/
  ID3D12Resource *directionalLightResource =
      CreateBufferResource(device, sizeof(DirectionalLight));

  // データを書き込む
  DirectionalLight *directionalLightData = nullptr;

  // 書き込むためのアドレスを取得
  directionalLightResource->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData));

  // Lightingの色
  directionalLightData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  directionalLightData->direction = Normalize(Vector3(0.0f, -1.0f, 0.0f));
  directionalLightData->intensity = 1.0f;

  // シリアライズしてバイナリにする
  ID3DBlob *signatureBlob = nullptr;
  ID3DBlob *errorBlob = nullptr;
  hr = D3D12SerializeRootSignature(&descroptionRootSignature,
                                   D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob,
                                   &errorBlob);

  if (FAILED(hr)) {
    Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
    assert(false);
  }

  // バイナリを元に生成
  ID3D12RootSignature *rootSignature = nullptr;
  hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                   signatureBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&rootSignature));
  assert(SUCCEEDED(hr));

  // inputLayout
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
  inputElementDescs[0].SemanticName = "POSITION";
  inputElementDescs[0].SemanticIndex = 0;
  inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

  inputElementDescs[1].SemanticName = "TEXCOORD";
  inputElementDescs[1].SemanticIndex = 0;
  inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
  inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

  inputElementDescs[2].SemanticName = "NORMAL";
  inputElementDescs[2].SemanticIndex = 0;
  inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
  inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

  D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
  inputLayoutDesc.pInputElementDescs = inputElementDescs;
  inputLayoutDesc.NumElements = _countof(inputElementDescs);

  // BlendStateの設定
  D3D12_BLEND_DESC blendDesc{};

  // すべての色要素を書き込む
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;

  // RasiterzerStateの設定
  D3D12_RASTERIZER_DESC rasterizerDesc{};

  // 裏面(時計回り)を表示しない
  rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

  // 三角形の中を塗りつぶす
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

  // shaderをcompileする
  IDxcBlob *vertexShaderBlob = CompileShader(
      L"Object3D.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
  assert(vertexShaderBlob != nullptr);

  IDxcBlob *pixelShaderBlob = CompileShader(
      L"Object3D.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
  assert(pixelShaderBlob != nullptr);

  // DepthStencilStateの設定
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

  // Depthの機能を有効化する
  depthStencilDesc.DepthEnable = true;

  // 書き込み
  depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

  // 比較関数はLessEqual。近ければ描画される
  depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  // PSOの生成
  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipeLineStateDesc{};
  graphicsPipeLineStateDesc.pRootSignature = rootSignature; // RootSignature
  graphicsPipeLineStateDesc.InputLayout = inputLayoutDesc;  // InputLayout
  graphicsPipeLineStateDesc.VS = {
      vertexShaderBlob->GetBufferPointer(),
      vertexShaderBlob->GetBufferSize()}; // VertexShader
  graphicsPipeLineStateDesc.PS = {
      pixelShaderBlob->GetBufferPointer(),
      pixelShaderBlob->GetBufferSize()};                      // PixelShader
  graphicsPipeLineStateDesc.BlendState = blendDesc;           // BlendState
  graphicsPipeLineStateDesc.RasterizerState = rasterizerDesc; // RasterizerState
  graphicsPipeLineStateDesc.DepthStencilState = depthStencilDesc;
  graphicsPipeLineStateDesc.DSVFormat =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // 資料20pまで

  // 書き込むRTVの情報
  graphicsPipeLineStateDesc.NumRenderTargets = 1;
  graphicsPipeLineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

  // 利用するトポロジ(形状)のタイプ。三角形
  graphicsPipeLineStateDesc.PrimitiveTopologyType =
      D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  // どのように画面に色を打ち込むかの設定
  graphicsPipeLineStateDesc.SampleDesc.Count = 1;
  graphicsPipeLineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  // 実際に生成
  ID3D12PipelineState *graphicsPipeLineState = nullptr;
  hr = device->CreateGraphicsPipelineState(
      &graphicsPipeLineStateDesc, IID_PPV_ARGS(&graphicsPipeLineState));
  assert(SUCCEEDED(hr));

  // 頂点リソース用のヒープの設定
  D3D12_HEAP_PROPERTIES uploadHeapProperties{};
  uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使う

  // 頂点リソースの設定
  D3D12_RESOURCE_DESC vertexResourceDesc{};

  // バッファリソース。テクスチャの場合はまた別の設定をする
  vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  vertexResourceDesc.Width =
      sizeof(VertexData) * 6; // リソースのサイズ。今回はVector4を三頂点分

  // バッファの場合はこれらを1にする決まり
  vertexResourceDesc.Height = 1;
  vertexResourceDesc.DepthOrArraySize = 1;
  vertexResourceDesc.MipLevels = 1;
  vertexResourceDesc.SampleDesc.Count = 1;

  // バッファの場合はこれにする決まり
  vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  // 実際に頂点リソースを作る
  ID3D12Resource *vertexResource =
      CreateBufferResource(device, sizeof(VertexData) * 1536);

  // 頂点バッファビューを作成する
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

  // リソースの先頭アドレスから使う
  vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();

  // 使用するリソースのサイズは頂点三つ分のサイズ
  vertexBufferView.SizeInBytes = sizeof(VertexData) * 1536;

  // 1頂点あたりのサイズ
  vertexBufferView.StrideInBytes = sizeof(VertexData);

  // Sprite用のリソースを作成する
  ID3D12Resource *vertexResourceSprite =
      CreateBufferResource(device, sizeof(VertexData) * 6);

  // 頂点バッファビューを作成する
  D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};

  // リソースの先頭アドレスから使う
  vertexBufferViewSprite.BufferLocation =
      vertexResourceSprite->GetGPUVirtualAddress();

  // 使用するリソースサイズは頂点六つ分のサイズ
  vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;

  // 1頂点あたりのサイズ
  vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

  // 頂点リソースにデータを書き込む
  VertexData *vertexData = nullptr;

  // 書き込むためのアドレスを取得
  vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

  //=============================
  // 頂点インデックス(球)
  //=============================
  ID3D12Resource *indexResource =
      CreateBufferResource(device, sizeof(uint32_t) * 1536);

  // インデックスバッファビュー
  D3D12_INDEX_BUFFER_VIEW indexBufferView{};

  // リソースの先頭アドレスから使う
  indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();

  // 使用するリソース
  indexBufferView.SizeInBytes = sizeof(uint32_t) * 1536;

  // インデックスはuint32_t
  indexBufferView.Format = DXGI_FORMAT_R32_UINT;

  uint32_t *indexData = nullptr;

  indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));

  //=============================
  // 球
  //=============================

  const uint32_t kSubdivision = 15; // 分割数

  uint32_t latIndex;
  uint32_t lonIndex;

  // 経度分割一つ分の角度
  const float kLonEvery =
      static_cast<float>(M_PI) * 2.0f / static_cast<float>(kSubdivision);

  // 緯度
  const float kLatEvery =
      static_cast<float>(M_PI) / static_cast<float>(kSubdivision);

  // 緯度の方向に分割
  for (latIndex = 0; latIndex < kSubdivision; ++latIndex) {

    // 現在の緯度
    float lat = static_cast<float>(M_PI) / 2.0f + kLatEvery * latIndex;

    // 経度の方向に分割
    for (lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {

      // 現在の経度
      float lon = lonIndex * kLonEvery;

      // 最初に書き込む場所(a)
      uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;

      // texcoord
      float u0 =
          static_cast<float>(lonIndex) / static_cast<float>(kSubdivision);

      float v0 =
          static_cast<float>(latIndex) / static_cast<float>(kSubdivision);

      float u1 = float(lonIndex + 1) / kSubdivision;
      float v1 = float(latIndex + 1) / kSubdivision;

      // 頂点データ入力

      // a 左下
      vertexData[start].position.x = cosf(lat) * cosf(lon);
      vertexData[start].position.y = sinf(lat);
      vertexData[start].position.z = cosf(lat) * sinf(lon);
      vertexData[start].position.w = 1.0f;
      vertexData[start].texcoord = {u0, v0};
      vertexData[start].normal.x = vertexData[start].position.x;
      vertexData[start].normal.y = vertexData[start].position.y;
      vertexData[start].normal.z = vertexData[start].position.z;

      // c 右下
      vertexData[start + 1].position.x = cosf(lat) * cosf(lon + kLonEvery);
      vertexData[start + 1].position.y = sinf(lat);
      vertexData[start + 1].position.z = cosf(lat) * sinf(lon + kLonEvery);
      vertexData[start + 1].position.w = 1.0f;
      vertexData[start + 1].texcoord = {u1, v0};
      vertexData[start + 1].normal.x = vertexData[start + 1].position.x;
      vertexData[start + 1].normal.y = vertexData[start + 1].position.y;
      vertexData[start + 1].normal.z = vertexData[start + 1].position.z;

      // b 左上
      vertexData[start + 2].position.x = cosf(lat + kLatEvery) * cosf(lon);
      vertexData[start + 2].position.y = sinf(lat + kLatEvery);
      vertexData[start + 2].position.z = cosf(lat + kLatEvery) * sinf(lon);
      vertexData[start + 2].position.w = 1.0f;
      vertexData[start + 2].texcoord = {u0, v1};
      vertexData[start + 2].normal.x = vertexData[start + 2].position.x;
      vertexData[start + 2].normal.y = vertexData[start + 2].position.y;
      vertexData[start + 2].normal.z = vertexData[start + 2].position.z;

      //// c 右下
      // vertexData[start + 3].position.x = cosf(lat) * cosf(lon + kLonEvery);
      // vertexData[start + 3].position.y = sinf(lat);
      // vertexData[start + 3].position.z = cosf(lat) * sinf(lon + kLonEvery);
      // vertexData[start + 3].position.w = 1.0f;
      // vertexData[start + 3].texcoord = {u1, v0};
      // vertexData[start + 3].normal.x = vertexData[start + 3].position.x;
      // vertexData[start + 3].normal.y = vertexData[start + 3].position.y;
      // vertexData[start + 3].normal.z = vertexData[start + 3].position.z;

      // d 右上
      vertexData[start + 3].position.x =
          cosf(lat + kLatEvery) * cosf(lon + kLonEvery);
      vertexData[start + 3].position.y = sinf(lat + kLatEvery);
      vertexData[start + 3].position.z =
          cosf(lat + kLatEvery) * sinf(lon + kLonEvery);
      vertexData[start + 3].position.w = 1.0f;
      vertexData[start + 3].texcoord = {u1, v1};
      vertexData[start + 3].normal.x = vertexData[start + 3].position.x;
      vertexData[start + 3].normal.y = vertexData[start + 3].position.y;
      vertexData[start + 3].normal.z = vertexData[start + 3].position.z;

      // 頂点インデックスデータに入力
      indexData[start] = start + 0;
      indexData[start + 1] = start + 1;
      indexData[start + 2] = start + 2;
      indexData[start + 3] = start + 1;
      indexData[start + 4] = start + 3;
      indexData[start + 5] = start + 2;

      //// b 左上
      // vertexData[start + 5].position.x = cosf(lat + kLatEvery) * cosf(lon);
      // vertexData[start + 5].position.y = sinf(lat + kLatEvery);
      // vertexData[start + 5].position.z = cosf(lat + kLatEvery) * sinf(lon);
      // vertexData[start + 5].position.w = 1.0f;
      // vertexData[start + 5].texcoord = {u0, v1};
      // vertexData[start + 5].normal.x = vertexData[start + 5].position.x;
      // vertexData[start + 5].normal.y = vertexData[start + 5].position.y;
      // vertexData[start + 5].normal.z = vertexData[start + 5].position.z;
    }
  }

  //=============================
  // スプライト
  //=============================

  // 頂点データ設定
  VertexData *vertexDataSprite = nullptr;

  vertexResourceSprite->Map(0, nullptr,
                            reinterpret_cast<void **>(&vertexDataSprite));

  // 一枚目の三角形
  vertexDataSprite[0].position = {0.0f, 360.0f, 0.0f, 1.0f}; // 左下
  vertexDataSprite[0].texcoord = {0.0f, 1.0f};
  vertexDataSprite[0].normal = {0.0f, 0.0f, -1.0f};
  vertexDataSprite[1].position = {0.0f, 0.0f, 0.0f, 1.0f}; // 左上
  vertexDataSprite[1].texcoord = {0.0f, 0.0f};
  vertexDataSprite[1].normal = {0.0f, 0.0f, -1.0f};
  vertexDataSprite[2].position = {640.0f, 360.0f, 0.0f, 1.0f}; // 右下
  vertexDataSprite[2].texcoord = {1.0f, 1.0f};
  vertexDataSprite[2].normal = {0.0f, 0.0f, -1.0f};

  // 二枚目
  vertexDataSprite[3].position = {640.0f, 0.0f, 0.0f, 1.0f}; // 右上
  vertexDataSprite[3].texcoord = {1.0f, 0.0f};
  vertexDataSprite[3].normal = {0.0f, 0.0f, -1.0f};

  //=============================
  // 頂点インデックス(スプライト)
  //=============================
  ID3D12Resource *indexResourceSprite =
      CreateBufferResource(device, sizeof(uint32_t) * 6);

  // インデックスビュー
  D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};

  // リソースの先頭アドレスから使う
  indexBufferViewSprite.BufferLocation =
      indexResourceSprite->GetGPUVirtualAddress();

  // 使用するリソースのサイズはインデックス六つ分
  indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;

  // インデックスはuint32_tとする
  indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

  // データを書き込む
  uint32_t *indexDataSprite = nullptr;

  indexResourceSprite->Map(0, nullptr,
                           reinterpret_cast<void **>(&indexDataSprite));

  indexDataSprite[0] = 0; // 左下
  indexDataSprite[1] = 1; // 左上
  indexDataSprite[2] = 2; // 右下
  indexDataSprite[3] = 1;
  indexDataSprite[4] = 3; // 右上
  indexDataSprite[5] = 2;

  // ビューポート
  D3D12_VIEWPORT viewport{};

  // クライアント領域のサイズと一緒にして画面全体に表示
  viewport.Width = kClientWidth;
  viewport.Height = kClientHeight;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;

  // シザー矩形
  D3D12_RECT scissorRect{};

  // 基本的にビューポートと同じ矩形が構成されるようにする
  scissorRect.left = 0;
  scissorRect.right = kClientWidth;
  scissorRect.top = 0;
  scissorRect.bottom = kClientHeight;

  MSG msg{};

  // Transform変数を作る
  Transform transform{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  // カメラ行列
  Transform cameraTransform{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, -10.0f},
  };

  // Sprite用のTransform
  Transform transformSprite{
      {1.0f, 1.0f, 1.0},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  // UVTransfotm用
  Transform uvTransformSprite{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  // texture切り替え用
  bool useMonsterBall = true;

  // Lighting
  bool enableLighting = true;

  // LightingDirection用
  Vector3 tempDirection = directionalLightData->direction;

  // ImGuiの初期化
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX12_Init(device, swapChainDesc.BufferCount, rtvDesc.Format,
                      srvDescriptorHeap,
                      srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                      srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()

  );

  // ウィンドウのxボタンが押されるまでループ
  while (msg.message != WM_QUIT) {
    // Windowにメッセージが来てたら最優先で処理させる
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {

      ImGui_ImplDX12_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();

      // ゲームの処理

      transform.rotate.y += 0.03f;

      // 開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
      ImGui::ColorEdit3("materialColor", &materialData->color.x);
      ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
      ImGui::Checkbox("useMonsterBall", &useMonsterBall);
      ImGui::Checkbox("enableLighting", &enableLighting);
      ImGui::ColorEdit3("LightingColor", &directionalLightData->color.x);
      ImGui::DragFloat3("Lighting Direction", &tempDirection.x, 0.01f, -1.0f,
                        1.0f);
      ImGui::DragFloat("Intensity", &directionalLightData->intensity, 0.001f,
                       0.0f, 1.0f);
      ImGui::DragFloat3("cameraTranslate", &cameraTransform.translate.x, 0.01f);
      ImGui::DragFloat3("cameraRotate", &cameraTransform.rotate.x, 0.001f);

      ImGui::DragFloat3("UVTranslate", &uvTransformSprite.translate.x, 0.01f,
                        -10.0f, 10.0f);
      ImGui::DragFloat3("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f,
                        10.0f);
      ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

      materialData->enableLighting = enableLighting;
      directionalLightData->direction = Normalize(tempDirection);

      // ImGuiの内部コマンドを生成する
      ImGui::Render();

      Matrix4x4 worldMatrix = MakeAffineMatrix(
          transform.scale, transform.rotate, transform.translate);
      Matrix4x4 cameraMatrix =
          MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate,
                           cameraTransform.translate);
      Matrix4x4 viewMatrix = Inverse(cameraMatrix);
      Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
          0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
      Matrix4x4 worldViewProjectionMatrix =
          Multiply(Multiply(worldMatrix, viewMatrix), projectionMatrix);

      transformationMatrixData->WVP = worldViewProjectionMatrix;
      transformationMatrixData->World = worldMatrix;

      // Sprite用のWorldViewProjectionMatrixを作る
      Matrix4x4 worldMatrixSprite =
          MakeAffineMatrix(transformSprite.scale, transformSprite.rotate,
                           transformSprite.translate);
      Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
      Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(
          0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
      Matrix4x4 worldViewProjectionMatrixSprite =
          Multiply(Multiply(worldMatrixSprite, viewMatrixSprite),
                   projectionMatrixSprite);

      Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
      uvTransformMatrix = Multiply(
          uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
      uvTransformMatrix = Multiply(
          uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
      materialDataSprite->uvTransform = uvTransformMatrix;

      transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
      transformationMatrixDataSprite->World = worldMatrixSprite;

      // これから書き込むバックバッファのインデックスを取得
      UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

      // TransitionBarrierの設定
      D3D12_RESOURCE_BARRIER barrier{};

      // 今回のバリアはTransition
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

      // Noneにしておく
      barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

      // バリアを張る対象のリソース。現在のバックバッファに対して行う
      barrier.Transition.pResource = swapChainResources[backBufferIndex];

      // 遷移前(現在)のResourceState
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

      // 遷移後のResourceState
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

      // TransitionBarrierを張る
      commandList->ResourceBarrier(1, &barrier);

      // 描画先のRTVとDSVを設定する
      D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
          GetCPUDescriptorHandle(dsvDescriptorHeap, descriptorSizeDSV, 0);
      commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false,
                                      &dsvHandle);

      // 指定した色で画面全体をクリアする
      float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f}; // 青っぽい色、RGBAの順
      commandList->ClearRenderTargetView(rtvHandles[backBufferIndex],
                                         clearColor, 0, nullptr);

      commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH,
                                         1.0f, 0, 0, nullptr);

      // 描画用のDescriptorHeapの設定
      ID3D12DescriptorHeap *descriptorHeaps[] = {srvDescriptorHeap};
      commandList->SetDescriptorHeaps(1, descriptorHeaps);

      commandList->RSSetViewports(1, &viewport);       // Viewportを設定
      commandList->RSSetScissorRects(1, &scissorRect); // Scissorを設定

      // RootSignatureを設定。PSOに設定しているけど別途設定が必要
      commandList->SetGraphicsRootSignature(rootSignature);
      commandList->SetPipelineState(graphicsPipeLineState);     // PSOを設定
      commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定

      // 形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばいい
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      commandList->IASetIndexBuffer(&indexBufferView);

      // マテリアルCBufferの場所を設定
      commandList->SetGraphicsRootConstantBufferView(
          0, materialResource->GetGPUVirtualAddress());

      // wvp用のCBufferの場所を設定
      commandList->SetGraphicsRootConstantBufferView(
          1, transformationMatrixResource->GetGPUVirtualAddress());

      // SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
      commandList->SetGraphicsRootDescriptorTable(
          2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);

      // Lighting
      commandList->SetGraphicsRootConstantBufferView(
          3, directionalLightResource->GetGPUVirtualAddress());

      // 描画(DrawCall/ドローコール)。3頂点で1つのインスタンス
      /*commandList->DrawInstanced(1536, 1, 0, 0);*/
      commandList->DrawIndexedInstanced(1536, 1, 0, 0, 0);

      // Spriteの描画
      commandList->IASetVertexBuffers(0, 1,
                                      &vertexBufferViewSprite); // VBVを設定

      commandList->IASetIndexBuffer(&indexBufferViewSprite); // IBVを設定

      // TransformationMatrixCBufferの場所を設定
      commandList->SetGraphicsRootConstantBufferView(
          1, transformationMatrixResourceSprite->GetGPUVirtualAddress());

      // Spriteは常に"uvChecker"にする
      commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

      // マテリアルCBufferの場所を設定。球とは別のマテリアルを使う
      commandList->SetGraphicsRootConstantBufferView(
          0, materialResourceSprite->GetGPUVirtualAddress());

      // ドローコール
      commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

      // 実際のcommandListのImGuiの描画コマンドを積む
      ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

      // 画面に描く処理はすべて終わり、画面に映すので状態を遷移
      // 今回はRenderTargetからPresentにする
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;

      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

      // TransitionBarrierを張る
      commandList->ResourceBarrier(1, &barrier);

      // コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
      hr = commandList->Close();
      assert(SUCCEEDED(hr));

      // GPUにコマンドリストの実行を行わせる
      ID3D12CommandList *commandLists[] = {commandList};
      commandQueue->ExecuteCommandLists(1, commandLists);

      // GPUとOSに画面の交換を行うように通知する
      swapChain->Present(1, 0);

      // Fenceの値を更新
      fenceValue++;

      // GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
      commandQueue->Signal(fence, fenceValue);

      // Fenceの値が指定したSignal値にたどり着いたか確認する
      // GetCompletedValueの初期値はFenceの作成時に渡した初期値
      if (fence->GetCompletedValue() < fenceValue) {

        // 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
        fence->SetEventOnCompletion(fenceValue, fenceEvent);

        // イベントを待つ
        WaitForSingleObject(fenceEvent, INFINITE);
      }

      // 次のフレーム用のコマンドリストを準備
      hr = commandAllocator->Reset();
      assert(SUCCEEDED(hr));
      hr = commandList->Reset(commandAllocator, nullptr);
      assert(SUCCEEDED(hr));
    }
  }

  // ImGuiの終了処理
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  // 解放処理
  CloseHandle(fenceEvent);
  fence->Release();
  rtvDescriptorHeap->Release();
  srvDescriptorHeap->Release();
  swapChainResources[0]->Release();
  swapChainResources[1]->Release();
  swapChain->Release();
  commandList->Release();
  commandAllocator->Release();
  commandQueue->Release();
  device->Release();
  useAdapter->Release();
  dxgiFactory->Release();

  vertexResource->Release();
  graphicsPipeLineState->Release();
  signatureBlob->Release();
  if (errorBlob) {
    errorBlob->Release();
  }
  rootSignature->Release();
  pixelShaderBlob->Release();
  vertexShaderBlob->Release();

  materialResource->Release();
  transformationMatrixResource->Release();

  textureResource->Release();
  textureResource2->Release();

  depthStencilResource->Release();

  dsvDescriptorHeap->Release();

  vertexResourceSprite->Release();

  transformationMatrixResourceSprite->Release();

  materialResourceSprite->Release();

  directionalLightResource->Release();

  indexResourceSprite->Release();

  indexResource->Release();

  CoUninitialize();

#ifdef _DEBUG

  debugController->Release();

#endif // DEBUG

  CloseWindow(hwnd);

  // リソースリークチェック
  IDXGIDebug1 *debug;

  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {

    debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
    debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);

    debug->Release();
  }

  return 0;
}
