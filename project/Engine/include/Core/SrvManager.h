#pragma once
#include "Core/Dx12Core.h"

class SrvManager {

public:
  void Initialize(Dx12Core *dx12Core);

  uint32_t Allocate();

  D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
  D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

  // SRV生成（テクスチャ用）
  void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource *pResource,
                             DXGI_FORMAT format, UINT mipLevels);

  // SRV生成（Structured Buffer用）
  void CreateSRVforStructuredBuffer(uint32_t srvIndex,
                                    ID3D12Resource *pResource, UINT numElements,
                                    UINT structureByteStride);

  void PreDraw();

  void SetGraphicsRootDescriptorTable(UINT rootParameterIndex,
                                      uint32_t srvIndex);

  bool CanAllocate();

private:
  Dx12Core *dx12Core_ = nullptr;

  // 最大SRV数(最大テクスチャ枚数)
  static const uint32_t kMaxSRVCount;

  // SRV用のディスクリプタサイズ
  uint32_t desctiptorSize;

  // SRV用ディスクリプタヒープ
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> desctiptorHeap;

  uint32_t useIndex_ = 0;
};
