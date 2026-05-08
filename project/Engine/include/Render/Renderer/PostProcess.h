#pragma once
#include "Core/Dx12Core.h"
#include <wrl.h>

class SrvManager;

class PostProcess {
public:
  void Initialize(Dx12Core* dx12Core);
  void Draw(uint32_t srvIndex, SrvManager* srvManager);

private:
  Dx12Core* dx12Core_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

  void CreateRootSignature();
  void CreatePSO();
};
