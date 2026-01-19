#pragma once
#include "Math/MathUtil.h"
#include <cstdint>
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

class ModelRenderer;

class Dx12Core;

class Model {

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
    float shininess;
    float padding2[3];
  };

  struct MaterialData {
    std::string textureFilePath;
    // uint32_t textureIndex = 0;
  };

  struct ModelData {
    std::vector<VertexData> vertices;
    MaterialData material;
  };

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(ModelRenderer *modelRenderer,
                  const std::string &directorypath,
                  const std::string &filename);

  /// <summary>
  /// 描画
  /// </summary>
  void Draw();

private:
  ModelRenderer *modelRenderer_;

  Dx12Core *dx12Core_ = nullptr;

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
};
