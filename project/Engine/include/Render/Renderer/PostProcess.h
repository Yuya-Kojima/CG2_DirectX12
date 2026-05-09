#pragma once
#include "Core/Dx12Core.h"
#include <wrl.h>
#include "Math/Matrix4x4.h"

class SrvManager;

class PostProcess {
public:
  void Initialize(Dx12Core* dx12Core);
  void Draw(uint32_t renderSrvIndex, uint32_t depthSrvIndex, uint32_t maskSrvIndex, SrvManager* srvManager);

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
  void SetDepthOutlineWeight(float weight) { depthOutlineWeight_ = weight; }
  void SetDepthOutlineAttenuation(float attenuation) { depthOutlineAttenuation_ = attenuation; }
  void SetRadialBlurCenter(float x, float y) { radialBlurCenter_[0] = x; radialBlurCenter_[1] = y; }
  void SetRadialBlurWidth(float width) { radialBlurWidth_ = width; }
  void SetRadialBlurSamples(int samples) { radialBlurSamples_ = samples; }
  void SetRadialBlurInnerRadius(float radius) { radialBlurInnerRadius_ = radius; }
  void SetRadialBlurOuterRadius(float radius) { radialBlurOuterRadius_ = radius; }
  void SetRadialBlurAberration(float aberration) { radialBlurAberration_ = aberration; }
  void SetDissolveThreshold(float threshold) { dissolveThreshold_ = threshold; }
  void SetDissolveEdgeRange(float range) { dissolveEdgeRange_ = range; }
  void SetDissolveEdgeColor(float r, float g, float b) {
      dissolveEdgeColor_[0] = r; dissolveEdgeColor_[1] = g; dissolveEdgeColor_[2] = b;
  }
  void SetProjectionInverse(const Matrix4x4& m) { projectionInverse_ = m; }

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
  
  float depthOutlineWeight_ = 10.0f;
  float depthOutlineAttenuation_ = 0.05f;

  float radialBlurCenter_[2] = {0.5f, 0.5f};
  float radialBlurWidth_ = 0.01f;
  int32_t radialBlurSamples_ = 10;
  float radialBlurInnerRadius_ = 0.0f;
  float radialBlurOuterRadius_ = 0.5f;
  float radialBlurAberration_ = 0.1f;

  float dissolveThreshold_ = 0.5f;
  float dissolveEdgeRange_ = 0.05f;
  float dissolveEdgeColor_[3] = {1.0f, 0.4f, 0.3f};

  Matrix4x4 projectionInverse_;

  Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

  void CreateRootSignature();
  void CreatePSO();
};
