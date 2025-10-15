#include "Model.h"
#include "TextureManager.h"

void Model::Initialize(ModelRenderer *modelRenderer) {

  modelRenderer_ = modelRenderer;

  LoadModelFile("resources", "plane.obj");

  CreateVertexData();

  CreateMaterialData();

  // objデータが参照しているテクスチャ読み込み
  TextureManager::GetInstance()->LoadTexture(
      modelData_.material.textureFilePath);

  // 読み込んだテクスチャの番号を取得
  modelData_.material.textureIndex =
      TextureManager::GetInstance()->GetTextureIndexByFilePath(
          modelData_.material.textureFilePath);
}
void Model::Draw() {

    

  commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定

  // マテリアルCBufferの場所を設定
  commandList->SetGraphicsRootConstantBufferView(
      0, materialResource->GetGPUVirtualAddress());

  // SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
  commandList->SetGraphicsRootDescriptorTable(
      2, TextureManager::GetInstance()->GetSrvHandleGPU(1));

  // 描画(DrawCall/ドローコール)。3頂点で1つのインスタンス
  commandList->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}
