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
  void SetUseVignette(bool useVignette) { useVignette_ = useVignette; }
  void SetVignetteScale(float scale) { vignetteScale_ = scale; }
  void SetVignetteExponent(float exponent) { vignetteExponent_ = exponent; }
  void SetPostEffectType(int type) { postEffectType_ = type; }
  void SetBoxFilterK(int k) { boxFilterK_ = k; }
  void SetGaussianFilterK(int k) { gaussianFilterK_ = k; }
  void SetGaussianSigma(float sigma) { gaussianSigma_ = sigma; }

private:
  Dx12Core* dx12Core_ = nullptr;
  int postEffectType_ = 0; // 0: None, 1: BoxFilter, 2: GaussianFilter
  int boxFilterK_ = 1;     // K radius for BoxFilter (1=3x3, 2=5x5...)
  int gaussianFilterK_ = 1; // K radius for GaussianFilter
  float gaussianSigma_ = 4.0f; // Sigma for GaussianFilter
  bool useGrayscale_ = false;
  float monotoneColor_[3] = {1.0f, 1.0f, 1.0f};

  bool useVignette_ = false;
  float vignetteScale_ = 16.0f;
  float vignetteExponent_ = 0.8f;

  Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

  void CreateRootSignature();
  void CreatePSO();
};
