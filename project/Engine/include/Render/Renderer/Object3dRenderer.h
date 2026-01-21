#pragma once
#include "Core/Dx12Core.h"
#include "Math/MathUtil.h"

class GameCamera;

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

public:
  Dx12Core *GetDx12Core() const { return dx12Core_; }

private:
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

private:
  GameCamera *defaultCamera = nullptr;

public:
  void SetDefaultCamera(GameCamera *camera) { defaultCamera = camera; }
  GameCamera *GetDefaultCamera() const { return defaultCamera; }

public:
  struct PointLight {
    Vector4 color;    // 色
    Vector3 position; // 位置
    float intensity;  // 輝度
    float radius;     // ライトの届く最大距離
    float decay;      // 減衰率
    float padding[2];
  };

  ID3D12Resource *GetPointLightResource() const {
    return pointLightResource_.Get();
  }

  PointLight *GetPointLightData() { return pointLightData_; }

private:
  /* PointLight用データ
　-----------------------------*/
  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_ = nullptr;

  // バッファリソース内のデータを指すポインタ
  Object3dRenderer::PointLight *pointLightData_ = nullptr;

  /// <summary>
  /// PointLightを生成
  /// </summary>
  void CreatePointLightData();
};
