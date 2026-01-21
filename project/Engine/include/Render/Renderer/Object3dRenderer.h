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
  struct DirectionalLight {
    Vector4 color;     // ライトの色
    Vector3 direction; // ライトの向き
    float intensity;   // 輝度
  };

  ID3D12Resource *GetDirectionalLightResource() {
    return directionalLightResource_.Get();
  }

  DirectionalLight *GetDirectionalLightData() { return directionalLightData_; }

private:
  /* 平行光源データ
 -----------------------------*/
  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_ = nullptr;

  // バッファリソース内のデータを指すポインタ
  DirectionalLight *directionalLightData_ = nullptr;

  /// <summary>
  /// 平行光源データを生成
  /// </summary>
  void CreateDirectionalLightData();

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

public:
  struct SpotLight {
    Vector4 color;
    Vector3 position;
    float intensity;
    Vector3 direction;
    float distance;
    float decay;
    float cosAngle; // スポットライトの余弦
    float padding[2];
  };

  ID3D12Resource *GetSpotLightResource() const {
    return spotLightResource_.Get();
  }

  SpotLight *GetSpotLightData() { return spotLightData_; }

private:
  /* SpotLight用データ
　-----------------------------*/
  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_ = nullptr;

  // バッファリソース内のデータを指すポインタ
  Object3dRenderer::SpotLight *spotLightData_ = nullptr;

  /// <summary>
  /// SpotLightを生成
  /// </summary>
  void CreateSpotLightData();
};
