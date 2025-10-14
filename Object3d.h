#pragma once
#include "Matrix4x4.h"
#include "Transform.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

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

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize();

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

  /* 座標変換行列データ
  -----------------------------*/

  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource = nullptr;

  // バッファリソース内のデータを指すポインタ
  TransformationMatrix *transformationMatrixData = nullptr;
};
