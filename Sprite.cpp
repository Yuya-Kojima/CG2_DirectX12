#include "Sprite.h"
#include "MatrixUtility.h"
#include "SpriteRenderer.h"
#include "TextureManager.h"

void Sprite::Initialize(SpriteRenderer *spriteRenderer, Dx12Core *dx12Core,
                        std::string textureFilePath) {

  spriteRenderer_ = spriteRenderer;

  dx12Core_ = dx12Core;

  textureIndex_ =
      TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

  // textureSrvHandleGPU_ = textureSrvHandleGPU;

  CreateVertexData();

  CreateMaterialData();

  CreateTransformationMatrixData();
}

void Sprite::Update(Transform uvTransform) {

  float left = 0.0f - anchorPoint.x;
  float right = 1.0f - anchorPoint.x;
  float top = 0.0f - anchorPoint.y;
  float bottom = 1.0f - anchorPoint.y;

  // 頂点リソースにデータを書き込む(4点分)

  // 一枚目の三角形
  // vertexData[0].position = {0.0f, 1.0f, 0.0f, 1.0f}; // 左下
  vertexData[0].position = {left, bottom, 0.0f, 1.0f}; // 左下
  vertexData[0].texcoord = {0.0f, 1.0f};
  vertexData[0].normal = {0.0f, 0.0f, -1.0f};
  //  vertexData[1].position = {0.0f, 0.0f, 0.0f, 1.0f}; // 左上
  vertexData[1].position = {left, top, 0.0f, 1.0f}; // 左上
  vertexData[1].texcoord = {0.0f, 0.0f};
  vertexData[1].normal = {0.0f, 0.0f, -1.0f};
  // vertexData[2].position = {1.0f, 1.0f, 0.0f, 1.0f}; // 右下
  vertexData[2].position = {right, bottom, 0.0f, 1.0f}; // 右下
  vertexData[2].texcoord = {1.0f, 1.0f};
  vertexData[2].normal = {0.0f, 0.0f, -1.0f};

  // 二枚目
  //  vertexData[3].position = {1.0f, 0.0f, 0.0f, 1.0f}; // 右上
  vertexData[3].position = {right, top, 0.0f, 1.0f}; // 右上
  vertexData[3].texcoord = {1.0f, 0.0f};
  vertexData[3].normal = {0.0f, 0.0f, -1.0f};

  // インデックスリソースにデータを書き込む(6点分)

  indexData[0] = 0; // 左下
  indexData[1] = 1; // 左上
  indexData[2] = 2; // 右下
  indexData[3] = 1;
  indexData[4] = 3; // 右上
  indexData[5] = 2;

  // Transform情報を作る
  Transform transform{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  transform.scale = {size.x, size.y, 1.0f};

  // 回転(スプライトなので基本Z回転のみ)
  transform.rotate = {0.0f, 0.0f, rotation};

  transform.translate = {position.x, position.y, 0.0f};

  // TransformからWorldMatrixを作る
  Matrix4x4 worldMatrix =
      MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

  // ViewMatrixを作って単位行列を代入
  Matrix4x4 viewMatrix = MakeIdentity4x4();

  // ProjectionMatrixを作って並列投影行列を書き込む
  Matrix4x4 projectionMatrix =
      MakeOrthographicMatrix(0.0f, 0.0f, float(WindowSystem::kClientWidth),
                             float(WindowSystem::kClientHeight), 0.0f, 100.0f);

  // Sprite用のWorldViewProjectionMatrixを作る
  Matrix4x4 worldViewProjectionMatrix =
      Multiply(Multiply(worldMatrix, viewMatrix), projectionMatrix);

  // uvTransformの情報
  Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransform.scale);
  uvTransformMatrix =
      Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransform.rotate.z));
  uvTransformMatrix =
      Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransform.translate));
  materialData->uvTransform = uvTransformMatrix;

  // データに代入
  transformationMatrixData->WVP = worldViewProjectionMatrix;
  transformationMatrixData->World = worldMatrix;
}

void Sprite::Draw() {

  ID3D12GraphicsCommandList *commandList_ = dx12Core_->GetCommandList();

  // VertexBufferViewを設定
  commandList_->IASetVertexBuffers(0, 1,
                                   &vertexBufferView); // VBVを設定

  // IndexBufferViewを設定
  commandList_->IASetIndexBuffer(&indexBufferView); // IBVを設定

  // マテリアルCBufferの場所を設定。球とは別のマテリアルを使う
  commandList_->SetGraphicsRootConstantBufferView(
      0, materialResource->GetGPUVirtualAddress());

  // TransformationMatrixCBufferの場所を設定
  commandList_->SetGraphicsRootConstantBufferView(
      1, transformationMatrixResource->GetGPUVirtualAddress());

  // SRVのDiscriptorTableの先頭を設定(描画に使うテクスチャの設定)
  commandList_->SetGraphicsRootDescriptorTable(
      2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex_));

  // 描画(ドローコール)
  commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::CreateVertexData() {

  // VertexResourceを作る
  //=========================
  vertexResource = dx12Core_->CreateBufferResource(sizeof(VertexData) * 6);

  // IndexResourceを作る
  //=========================
  indexResource = dx12Core_->CreateBufferResource(sizeof(uint32_t) * 6);

  // VertexBufferViewを作る(値を設定するだけ)
  //=======================================
  // リソースの先頭アドレスから使う
  vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();

  // 使用するリソースサイズは頂点六つ分のサイズ
  vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;

  // 1頂点あたりのサイズ
  vertexBufferView.StrideInBytes = sizeof(VertexData);

  // IndexBufferViewを作る(値を設定するだけ)
  //=======================================
  // リソースの先頭アドレスから使う
  indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();

  // 使用するリソースのサイズはインデックス六つ分
  indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;

  // インデックスはuint32_tとする
  indexBufferView.Format = DXGI_FORMAT_R32_UINT;

  // VertexResourceにデータを書き込むためのアドレスを取得してvertexDataに割り当てる
  //==========================================================================

  // 頂点データ設定
  vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

  // IndexResourceにデータを書き込むためのアドレスを取得してindexDataに割り当てる
  //==========================================================================

  indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
}

void Sprite::CreateMaterialData() {

  // リソースを作る
  materialResource = dx12Core_->CreateBufferResource(sizeof(Material));

  // 書き込むためのアドレスを取得
  materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));

  // マテリアルデータの初期値を書き込む
  materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  materialData->enableLighting = false;
  materialData->uvTransform = MakeIdentity4x4();
}

void Sprite::CreateTransformationMatrixData() {

  // リソースサイズ
  UINT transformationMatrixSize = (sizeof(TransformationMatrix) + 255) & ~255;

  // リソースを作る
  transformationMatrixResource =
      dx12Core_->CreateBufferResource(transformationMatrixSize);

  // 書き込むためのアドレスを取得
  transformationMatrixResource->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData));

  // 単位行列を書き込んでおく
  transformationMatrixData->World = MakeIdentity4x4();
  transformationMatrixData->WVP = MakeIdentity4x4();
}

void Sprite::ChangeTexture(std::string textureFilePath) {

  // まだ読み込まれていないテクスチャであれば読み込む
  TextureManager::GetInstance()->LoadTexture(textureFilePath);

  // テクスチャ番号を取得して差し替える
  textureIndex_ =
      TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
}
