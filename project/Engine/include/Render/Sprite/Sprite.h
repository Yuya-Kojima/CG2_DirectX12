#pragma once
#include "Core/WindowSystem.h"
#include "Math/Matrix4x4.h"
#include "Math/Transform.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>

class Dx12Core;

class SpriteRenderer;

class Sprite {

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
  void Initialize(SpriteRenderer *spriteRenderer,
                  const std::string &textureFilePath);

  /// <summary>
  /// 更新処理
  /// </summary>
  void Update(Transform uvTransform);

  /// <summary>
  /// 描画処理
  /// </summary>
  void Draw();

private:
  SpriteRenderer *spriteRenderer_ = nullptr;

  Dx12Core *dx12Core_ = nullptr;

  // テクスチャ番号
  // uint32_t textureIndex_ = 0;

  // テクスチャのファイルパス
  std::string textureFilePath_;

  // D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

private:
  /* 頂点データ
  -----------------------*/

  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
  Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;

  // バッファリソース内のデータを指すポインタ
  VertexData *vertexData = nullptr;
  uint32_t *indexData = nullptr;

  // バッファリソースの使い道を補足するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
  D3D12_INDEX_BUFFER_VIEW indexBufferView{};

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

  /// <summary>
  /// 頂点データを作成
  /// </summary>
  void CreateVertexData();

  /// <summary>
  /// マテリアルデータを作成
  /// </summary>
  void CreateMaterialData();

  /// <summary>
  /// 座標変換行列データを生成
  /// </summary>
  void CreateTransformationMatrixData();

private:
  // Position
  Vector2 position{0.0f, 0.0f};

  // Rotation
  float rotation = 0.0f;

  // size
  Vector2 size{640.0f, 360.0f};

  // アンカーポイント
  Vector2 anchorPoint = {0.0f, 0.0f};

public:
  /// <summary>
  /// positionのゲッター
  /// </summary>
  /// <returns></returns>
  const Vector2 &GetPosition() const { return position; }

  /// <summary>
  /// positionのセッター
  /// </summary>
  /// <param name="position"></param>
  void SetPosition(const Vector2 &position) { this->position = position; }

  /// <summary>
  /// rotationのゲッター
  /// </summary>
  /// <returns></returns>
  const float GetRotation() const { return rotation; }

  /// <summary>
  /// rotationのセッター
  /// </summary>
  /// <param name="rotation"></param>
  void SetRotation(float rotation) { this->rotation = rotation; }

  /// <summary>
  /// colorのゲッター
  /// </summary>
  /// <param name="rotation"></param>
  const Vector4 &GetColor() const { return materialData->color; }

  /// <summary>
  /// colorのセッター
  /// </summary>
  /// <param name="rotation"></param>
  void SetColor(const Vector4 &color) { materialData->color = color; }

  /// <summary>
  /// sizeのゲッター
  /// </summary>
  /// <returns></returns>
  const Vector2 &GetSize() const { return size; }

  /// <summary>
  /// sizeのセッター
  /// </summary>
  /// <param name="position"></param>
  void SetSize(const Vector2 &size) { this->size = size; }

  /// <summary>
  /// アンカーポイントのゲッター
  /// </summary>
  /// <returns></returns>
  Vector2 GetAnchorPoint() const { return anchorPoint; }

  /// <summary>
  /// アンカーポイントのセッター
  /// </summary>
  /// <param name="anchorPoint"></param>
  void SetAnchorPoint(Vector2 anchorPoint) { this->anchorPoint = anchorPoint; }

  /// <summary>
  /// 任意のタイミングでテクスチャを変更する
  /// </summary>
  /// <param name="textureFilePath"></param>
  void ChangeTexture(const std::string &textureFilePath);

  // フリップのゲッター
  bool GetIsFlipX() const { return isFlipX; }
  bool GetIsFlipY() const { return isFlipY; }

  // フリップのセッター
  void SetIsFlipX(const bool isFlipX) { this->isFlipX = isFlipX; }
  void SetIsFlipY(const bool isFlipY) { this->isFlipY = isFlipY; }

  /// <summary>
  /// テクスチャ左上座標ゲッター
  /// </summary>
  /// <returns></returns>
  Vector2 GetTextureLeftTop() const { return textureLeftTop; }

  /// <summary>
  /// テクスチャ左上座標セッター
  /// </summary>
  /// <param name="textureLeftTop"></param>
  void SetTextureLeftTop(Vector2 textureLeftTop) {
    this->textureLeftTop = textureLeftTop;
  }

  /// <summary>
  /// テクスチャ切り出しサイズゲッター
  /// </summary>
  /// <returns></returns>
  Vector2 GetTextureSize() const { return textureSize; }

  /// <summary>
  /// テクスチャ切り出しサイズセッター
  /// </summary>
  /// <param name="textureSize"></param>
  void SetTextureSize(Vector2 textureSize) { this->textureSize = textureSize; }

private:
  // 左右フリップ
  bool isFlipX = false;

  // 上下フリップ
  bool isFlipY = false;

private:
  // テクスチャ左上座標
  Vector2 textureLeftTop{0.0f, 0.0f};

  // テクスチャ切り出しサイズ
  Vector2 textureSize{100.0f, 100.0f};

  // テクスチャサイズをイメージに合わせる
  void AdjustTextureSize();
};
