#pragma once
#include "Camera/CameraForGPU.h"
#include "Core/Dx12Core.h"
#include "Math/MathUtil.h"
#include <d3d12.h>
#include <stdint.h>
#include <wrl.h>

class Model;

class Dx12Core;

class Object3dRenderer;

class GameCamera;

class Object3d {

  struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Matrix4x4 WorldInverseTranspose;
  };

  struct DirectionalLight {
    Vector4 color;     // ライトの色
    Vector3 direction; // ライトの向き
    float intensity;   // 輝度
  };

  struct PointLight {
    Vector4 color;    // 色
    Vector3 position; // 位置
    float intensity;  // 輝度
  };

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(Object3dRenderer *object3dRenderer);

  /// <summary>
  /// 更新
  /// </summary>
  void Update();

  /// <summary>
  /// 描画
  /// </summary>
  void Draw();

private:
  Object3dRenderer *object3dRenderer_ = nullptr;

  Dx12Core *dx12Core_ = nullptr;

  Model *model_;

public:
  void SetModel(const std::string &filePath);

private:
  /* 座標変換行列データ
  -----------------------------*/
  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource = nullptr;

  // バッファリソース内のデータを指すポインタ
  TransformationMatrix *transformationMatrixData = nullptr;

  /// <summary>
  /// 座標変換行列データを生成
  /// </summary>
  void CreateTransformationMatrixData();

  /* 平行光源データ
  -----------------------------*/
  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource = nullptr;

  // バッファリソース内のデータを指すポインタ
  DirectionalLight *directionalLightData = nullptr;

  /// <summary>
  /// 平行光源データを生成
  /// </summary>
  void CreateDirectionalLightData();

  /* 鏡面反射用データ
  -----------------------------*/
  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> cameraForGPUResource = nullptr;

  // バッファリソース内のデータを指すポインタ
  CameraForGPU *cameraForGPUData = nullptr;

  /// <summary>
  /// カメラデータを生成
  /// </summary>
  void CreateCameraForGPUData();

  /* PointLight用データ
　-----------------------------*/
  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource = nullptr;

  // バッファリソース内のデータを指すポインタ
  PointLight *pointLightData = nullptr;

  /// <summary>
  /// PointLightを生成
  /// </summary>
  void CreatePointLightData();

private:
  Vector3 scale_{1.0f, 1.0f, 1.0f};

  Vector3 rotate_{};

  Vector3 translate_{};

  Transform transform_{};

  // Transform cameraTransform_{};

public:
  // scale
  Vector3 GetScale() const { return scale_; }

  void SetScale(const Vector3 &scale) { scale_ = scale; }

  // rotation
  Vector3 GetRotation() const { return rotate_; }

  void SetRotation(const Vector3 &rotate) { rotate_ = rotate; }

  // translation
  Vector3 GetTranslation() const { return translate_; }

  void SetTranslation(const Vector3 &translate) { translate_ = translate; }

  // カメラ
  // void SetCameraMatrix(Transform cameraTransform) {
  //  cameraTransform_ = cameraTransform;
  //}

private:
  // uint32_t numInstance_{};

private:
  GameCamera *camera_ = nullptr;

public:
  void SetCamera(GameCamera *camera) { this->camera_ = camera; }
};
