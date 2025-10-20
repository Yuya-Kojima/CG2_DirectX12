#pragma once
#include "WindowSystem.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <array>
#include <chrono>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

class Dx12Core {
private:
  // DirectX12デバイス
  Microsoft::WRL::ComPtr<ID3D12Device> device;

  // DXGIファクトリー
  Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;

  // コマンドアロケータ
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

  // コマンドキュー
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;

  // コマンドリスト
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

  // WindowAPI
  WindowSystem *windowSystem = nullptr;

  // スワップチェーン
  Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;

  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

  // 深度バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = nullptr;

  // RTV用ヒープ
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
  uint32_t descriptorSizeRTV;

  // SRV用ヒープ
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
  uint32_t descriptorSizeSRV;

  // DSV用ヒープ
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
  uint32_t descriptorSizeDSV;

  // スワップチェーンからRTVに引っ張ってくるリソース(バックバッファ)
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

  // 取得したRTVハンドル
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

  // フェンスカウント
  uint64_t fenceValue;

  // フェンス
  Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;

  // フェンスイベント
  HANDLE fenceEvent;

  // ビューポート
  D3D12_VIEWPORT viewport{};

  // シザー矩形
  D3D12_RECT scissorRect{};

  // dxcユーティリティ
  IDxcUtils *dxcUtils = nullptr;

  // dxcコンパイラ
  IDxcCompiler3 *dxcCompiler = nullptr;

  // インクルードハンドラ
  IDxcIncludeHandler *includeHandler = nullptr;

  // リソースバリア
  D3D12_RESOURCE_BARRIER barrier{};

  /// <summary>
  /// CPUHandleの取得
  /// </summary>
  /// <param name="descriptorHeap">DescriptorHandle</param>
  /// <param name="descriptorSize">DescriptorSize</param>
  /// <param name="index">index</param>
  /// <returns></returns>
  static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
      uint32_t descriptorSize, uint32_t index);

  /// <summary>
  /// GPUHandleの取得
  /// </summary>
  /// <param name="descriptorHeap">descriptorHeap</param>
  /// <param name="descriptorSize">descriptorSize</param>
  /// <param name="index">index</param>
  /// <returns></returns>
  static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
      uint32_t descriptorSize, uint32_t index);

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(WindowSystem *windowSystem);

  /// <summary>
  /// 描画前処理
  /// </summary>
  void BeginFrame();

  /// <summary>
  /// 描画後処理
  /// </summary>
  void EndFrame();

  /// <summary>
  /// デバイスの初期化
  /// </summary>
  void InitializeDevice();

  /// <summary>
  /// コマンド関連の初期化
  /// </summary>
  void InitializeCommand();

  /// <summary>
  /// スワップチェーンの生成
  /// </summary>
  void InitializeSwapChain();

  /// <summary>
  /// 深度バッファの生成
  /// </summary>
  void InitializeDepthBuffer();

  /// <summary>
  /// ディスクリプタヒープの生成
  /// </summary>
  void InitializeDescriptorHeap();

  /// <summary>
  /// ディスクリプタヒープ生成関数
  /// </summary>
  /// <param name="heapType"></param>
  /// <param name="numDescriptors"></param>
  /// <param name="shaderVisible"></param>
  /// <returns></returns>
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
  CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors,
                       bool shaderVisible);

  /// <summary>
  /// RTVの初期化
  /// </summary>
  void InitializeRenderTargetView();

  D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

  D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

  /// <summary>
  /// DSVの初期化
  /// </summary>
  void InitializeDepthStencilView();

  /// <summary>
  /// フェンスの初期化
  /// </summary>
  void InitializeFence();

  /// <summary>
  /// ビューポート矩形の初期化
  /// </summary>
  void InitializeViewport();

  /// <summary>
  /// シザー矩形の初期化
  /// </summary>
  void InitializeScissorRect();

  /// <summary>
  /// DXCコンパイラの初期化
  /// </summary>
  void InitializeDXCcompiler();

  /// <summary>
  /// ImGuiの初期化
  /// </summary>
  void InitializeImGui();

  /// <summary>
  /// デバイスのゲッター
  /// </summary>
  /// <returns></returns>
  ID3D12Device *GetDevice() const { return device.Get(); }

  ID3D12GraphicsCommandList *GetCommandList() const {
    return commandList.Get();
  }

  /// <summary>
  /// シェーダーのコンパイル
  /// </summary>
  /// <param name="filePath">compileするshaderファイルへのパス</param>
  /// <param name="profile">compileに使用するprofile</param>
  /// <returns></returns>
  IDxcBlob *CompileShader(const std::wstring &filePath, const wchar_t *profile);

  /// <summary>
  /// リソースの生成
  /// </summary>
  /// <param name="device"></param>
  /// <param name="sizeInBytes"></param>
  /// <returns></returns>
  Microsoft::WRL::ComPtr<ID3D12Resource>
  CreateBufferResource(size_t sizeInBytes);

  /// <summary>
  /// TextureResourceを作る
  /// </summary>
  /// <param name="device"></param>
  /// <param name="metadata"></param>
  /// <returns></returns>
  Microsoft::WRL::ComPtr<ID3D12Resource>
  CreateTextureResource(const DirectX::TexMetadata &metadata);

  /// <summary>
  /// テクスチャデータの転送
  /// </summary>
  /// <param name="texture"></param>
  /// <param name="mipImages"></param>
  void UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture,
                         const DirectX::ScratchImage &mipImages);

    /// <summary>
  /// テクスチャファイルの読み込み
  /// </summary>
  /// <param name="filePath">テクスチャファイルパス</param>
  /// <returns>画像イメージデータ</returns>
  static DirectX::ScratchImage LoadTexture(const std::string &filePath);

  /// <summary>
  /// FPSを60固定する
  /// </summary>
  /// <param name="FPS"></param>
  void SetFPS(bool isLockFPS) {
    if (isLockFPS) {
      set60FPS = true;
    } else {
      set60FPS = false;
    }
  };

private:
  // FPS固定初期化
  void InitializeFixFPS();

  // FPS固定更新
  void UpdateFixFPS();

  // 記録時間
  std::chrono::steady_clock::time_point reference_;

  bool set60FPS = false;

public:
  // 最大SRV数(最大テクスチャ枚数)
  static const uint32_t kMaxSRVCount;
};
