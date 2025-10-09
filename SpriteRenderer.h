#pragma once
#include "Dx12Core.h"

class SpriteRenderer {

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(Dx12Core *dx12Core);

  Dx12Core *GetDx12Core() const { return dx12Core_; }

  /// <summary>
  /// 共通描画設定
  /// </summary>
  void Begin();

private:
  Dx12Core *dx12Core_ = nullptr;

  ID3D12Device *device_ = nullptr;

  ID3D12GraphicsCommandList *commandList_ = nullptr;

  // ルートシグネチャ
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

  // PSO
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState_ = nullptr;

  /// <summary>
  /// ルートシグネチャの生成
  /// </summary>
  void CreateSpriteRootSignature();

  /// <summary>
  /// PSOの生成
  /// </summary>
  void CreateSpritePSO();
};
