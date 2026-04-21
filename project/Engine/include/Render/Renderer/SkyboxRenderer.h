#pragma once
#include "Core/Dx12Core.h"

class SkyboxRenderer {

public:
  void Initialize(Dx12Core *dx12Core);
  void Begin();

private:
  Dx12Core *dx12Core_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

  void CreateRootSignature();
  void CreatePSO();

public:
  Dx12Core *GetDx12Core() const { return dx12Core_; }
};