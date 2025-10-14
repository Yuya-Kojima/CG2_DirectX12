#pragma once
#include "Dx12Core.h"

class Object3dRenderer {

public:
  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="dx12Core"></param>
  void Initialize(Dx12Core *dx12Core);

  /// <summary>
  /// 描画前共通部分処理
  /// </summary>
  void Begin();

private:
  Dx12Core *dx12Core_ = nullptr;

  // ルートシグネチャ
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

  // PSO
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState_ = nullptr;

  /// <summary>
  /// ルートシグネチャを作成
  /// </summary>
  void CreateRootSignature();

  /// <summary>
  /// オブジェクト用のPSOを作成
  /// </summary>
  void CreatePSO();
};
