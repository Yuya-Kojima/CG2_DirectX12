#pragma once
#include "Core/Dx12Core.h"
#include "Core/SrvManager.h"
#include "Render/Texture/RenderTexture.h"
#include <memory>

class Bloom {
public:
  void Initialize(Dx12Core* dx12Core, SrvManager* srvManager, uint32_t width, uint32_t height);
  
  /// <summary>
  /// ブルーム処理の実行
  /// </summary>
  /// <param name="dx12Core">Dx12Core</param>
  /// <param name="srvManager">SrvManager</param>
  /// <param name="srcSrvIndex">抽出元のテクスチャのSRVインデックス</param>
  /// <returns>最終的なボカシ結果（Yボカシ後）のSRVインデックス</returns>
  uint32_t Draw(Dx12Core* dx12Core, SrvManager* srvManager, uint32_t srcSrvIndex);

  // パラメータの設定
  void SetThreshold(float threshold) { threshold_ = threshold; }
  void SetSigma(float sigma) { sigma_ = sigma; }

  // ゲッター
  float GetThreshold() const { return threshold_; }
  float GetSigma() const { return sigma_; }

private:
  // 中間テクスチャ
  std::unique_ptr<RenderTexture> luminanceTexture_;
  std::unique_ptr<RenderTexture> blurXTexture_;
  std::unique_ptr<RenderTexture> blurYTexture_;
  
  float threshold_ = 0.8f;
  float sigma_ = 5.0f;
  
  uint32_t width_ = 1280;
  uint32_t height_ = 720;

  // 定数バッファの構造体
  struct ExtractData {
    float threshold;
    float padding[3];
  };

  struct BlurData {
    int32_t isHorizontal;
    float texWidth;
    float texHeight;
    float sigma;
  };

  // パイプライン関連 (Extract)
  Microsoft::WRL::ComPtr<ID3D12RootSignature> extractRootSignature_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> extractPipelineState_;
  Microsoft::WRL::ComPtr<ID3D12Resource> extractConstBuffer_;

  // パイプライン関連 (Blur)
  Microsoft::WRL::ComPtr<ID3D12RootSignature> blurRootSignature_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> blurPipelineState_;
  Microsoft::WRL::ComPtr<ID3D12Resource> blurConstBuffer_;

  void CreateExtractPipeline(Dx12Core* dx12Core);
  void CreateBlurPipeline(Dx12Core* dx12Core);
};
