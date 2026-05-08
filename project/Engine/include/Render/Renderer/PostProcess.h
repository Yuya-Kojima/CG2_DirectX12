#pragma once
#include "Core/Dx12Core.h"
#include <wrl.h>

class SrvManager;

class PostProcess {
public:
  void Initialize(Dx12Core* dx12Core);
  void Draw(uint32_t srvIndex, SrvManager* srvManager);

  void SetUseGrayscale(bool useGrayscale) { useGrayscale_ = useGrayscale; }
  void SetMonotoneColor(float r, float g, float b) {
    monotoneColor_[0] = r;
    monotoneColor_[1] = g;
    monotoneColor_[2] = b;
  }

private:
  Dx12Core* dx12Core_ = nullptr;
  bool useGrayscale_ = false;
  float monotoneColor_[3] = {1.0f, 1.0f, 1.0f};

  Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

  void CreateRootSignature();
  void CreatePSO();
};
