#pragma once
#include "Dx12Core.h"
#include "Matrix4x4.h"
#include "Transform.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include <d3d12.h>
#include <stdint.h>
#include <wrl.h>

class Dx12Core;

class Object3dRenderer;

class Object3d {

  struct VertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
  };

  struct Material {
    Vector4 color;
    int32_t enableLighting;
    float padding[3];
    Matrix4x4 uvTransform;
  };

  struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
  };

  struct DirectionalLight {
    Vector4 color;     // ライトの色
    Vector3 direction; // ライトの向き
    float intensity;   // 輝度
  };

  struct MaterialData {
    std::string textureFilePath;
    uint32_t textureIndex = 0;
  };

  struct ModelData {
    std::vector<VertexData> vertices;
    MaterialData material;
  };

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(Object3dRenderer *object3dRenderer, Dx12Core *dx12Core);

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

private:
  /* マテリアルデータ
  ----------------------------*/

  // マテリアルバッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;

  // バッファリソース内のデータを指すポインタ
  Material *materialData = nullptr;

  /// <summary>
  /// マテリアルデータを作成
  /// </summary>
  void CreateMaterialData();

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

  /* 頂点データ
  -----------------------------*/

  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;

  // バッファリソース内のデータを指すポインタ
  VertexData *vertexData = nullptr;

  // バッファリソースの使い道を補足するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

  /// <summary>
  /// 頂点データを生成
  /// </summary>
  void CreateVertexData();

  // Objファイルのデータ
  ModelData modelData_;

  /// <summary>
  /// mtlファイルを読む
  /// </summary>
  /// <param name="directoryPath"></param>
  /// <param name="filename"></param>
  /// <returns></returns>
  static MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
                                               const std::string &filename);

public:
  /// <summary>
  /// Objファイルを読む
  /// </summary>
  /// <param name="directoryPath"></param>
  /// <param name="filename"></param>
  /// <returns></returns>
  void LoadModelFile(const std::string &directoryPath,
                     const std::string &filename);

private:
  Vector3 scale_{};

  Vector3 rotation_{};

  Vector3 translation_{};

  Transform transform_{};

  Transform cameraTransform_{};

public:
  // scale
  Vector3 GetScale() const { return scale_; }

  void SetScale(Vector3 scale) { scale_ = scale; }

  // rotation
  Vector3 GetRotation() const { return rotation_; }

  void SetRotation(Vector3 rotation) { rotation_ = rotation; }

  // tranlation
  Vector3 GetTranslatoin() const { return translation_; }

  void SetTranslation(Vector3 translation) { translation_ = translation; }

  // カメラ
  void SetCameraMatrix(Transform cameraTransform) {
    cameraTransform_ = cameraTransform;
  }

private:
  uint32_t numInstance_{};
};
