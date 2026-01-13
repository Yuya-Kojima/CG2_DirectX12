#include "Core/SrvManager.h"

const uint32_t SrvManager::kMaxSRVCount = 512;

void SrvManager::Initialize(Dx12Core *dx12Core) {
  dx12Core_ = dx12Core;

  // ディスクリプタヒープの生成
  desctiptorHeap = dx12Core_->CreateDescriptorHeap(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);

  // ディスクリプタ一個分のサイズを取得して記録
  desctiptorSize = dx12Core_->GetDevice()->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

uint32_t SrvManager::Allocate() {

  assert(useIndex_ < kMaxSRVCount);

  // returnする番号を一旦記録しておく
  int index = useIndex_;

  // 次回のために番号を1進める
  useIndex_++;

  // 上で記録した番号をreturn
  return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index) {
  D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
      desctiptorHeap->GetCPUDescriptorHandleForHeapStart();
  handleCPU.ptr += (desctiptorSize * index);
  return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index) {
  D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
      desctiptorHeap->GetGPUDescriptorHandleForHeapStart();
  handleGPU.ptr += (desctiptorSize * index);
  return handleGPU;
}

void SrvManager::CreateSRVforTexture2D(uint32_t srvIndex,
                                       ID3D12Resource *pResource,
                                       DXGI_FORMAT format, UINT mipLevels) {

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = mipLevels;

  dx12Core_->GetDevice()->CreateShaderResourceView(
      pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVforStructuredBuffer(uint32_t srvIndex,
                                              ID3D12Resource *pResource,
                                              UINT numElements,
                                              UINT structureByteStride) {

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = DXGI_FORMAT_UNKNOWN;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  srvDesc.Buffer.FirstElement = 0;
  srvDesc.Buffer.NumElements = numElements;
  srvDesc.Buffer.StructureByteStride = structureByteStride;
  srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

  dx12Core_->GetDevice()->CreateShaderResourceView(
      pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::PreDraw() {

  // 描画用のDescriptorHeapの設定
  ID3D12DescriptorHeap *descriptorHeaps[] = {desctiptorHeap.Get()};
  dx12Core_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
}

void SrvManager::SetGraphicsRootDescriptorTable(UINT rootParameterIndex,
                                                uint32_t srvIndex) {
  dx12Core_->GetCommandList()->SetGraphicsRootDescriptorTable(
      rootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}

bool SrvManager::CanAllocate() { return useIndex_ < kMaxSRVCount; }
