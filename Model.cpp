#include "Model.h"
#include "MathUtil.h"
#include "ModelRenderer.h"
#include "TextureManager.h"
#include <fstream>

void Model::Initialize(ModelRenderer *modelRenderer,
                       const std::string &directorypath,
                       const std::string &filename) {

  modelRenderer_ = modelRenderer;

  dx12Core_ = modelRenderer_->GetDx12Core();

  LoadModelFile(directorypath, filename);

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

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
      dx12Core_->GetCommandList();

  commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定

  // マテリアルCBufferの場所を設定
  commandList->SetGraphicsRootConstantBufferView(
      0, materialResource->GetGPUVirtualAddress());

  // SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
  commandList->SetGraphicsRootDescriptorTable(
      2, TextureManager::GetInstance()->GetSrvHandleGPU(0));

  // 描画(DrawCall/ドローコール)。3頂点で1つのインスタンス
  commandList->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}

Model::MaterialData
Model::LoadMaterialTemplateFile(const std::string &directoryPath,
                                const std::string &filename) {
  // 1. 中で必要となる変数の宣言
  MaterialData materialData; // 構築するMaterialData
  std::string line;          // ファイルから読んだ一行を格納する

  // 2. ファイルを開く
  std::ifstream file(directoryPath + "/" + filename);
  assert(file.is_open()); // とりあえず開けなかったら止める

  // 3. 実際にファイルを読み、MaterialDataを構築していく
  while (std::getline(file, line)) {

    std::string identifier;
    std::istringstream s(line);

    s >> identifier;

    // identifierに応じた処理
    if (identifier == "map_Kd") {
      std::string textureFilename;
      s >> textureFilename;
      // 連結してファイルパスにする
      materialData.textureFilePath = directoryPath + "/" + textureFilename;
    }
  }

  // 4. MaterialDataを返す
  return materialData;
}

void Model::LoadModelFile(const std::string &directoryPath,
                          const std::string &filename) {

  // 1. 中で必要となる変数の宣言
  ModelData modelData; // 構築するModelData

  std::vector<Vector4> positions; // 位置
  std::vector<Vector3> normals;   // 法線
  std::vector<Vector2> texcoords; // テクスチャ座標
  std::string line;               // ファイルから読んだ一行を格納する

  VertexData triangle[3];

  // 2. ファイルを開く
  std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
  assert(file.is_open()); // とりあえず開けなかったら止める

  // 3. 実際にファイルを読み、ModelDataを構築していく
  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier; // 先頭の識別子を読む

    if (identifier == "v") {
      Vector4 position;

      s >> position.x >> position.y >> position.z;

      position.w = 1.0f;
      positions.push_back(position);
    } else if (identifier == "vt") {
      Vector2 texcoord;

      s >> texcoord.x >> texcoord.y;

      texcoords.push_back(texcoord);
    } else if (identifier == "vn") {
      Vector3 normal;

      s >> normal.x >> normal.y >> normal.z;

      normals.push_back(normal);
    } else if (identifier == "f") {

      // 面は三角形限定
      for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
        std::string vertexDefinition;
        s >> vertexDefinition;

        // 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
        std::istringstream v(vertexDefinition);

        uint32_t elementIndices[3];

        for (int32_t element = 0; element < 3; ++element) {
          std::string index;
          std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
          elementIndices[element] = std::stoi(index);
        }

        // 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
        Vector4 position = positions[elementIndices[0] - 1];
        Vector2 texcoord = texcoords[elementIndices[1] - 1];
        Vector3 normal = normals[elementIndices[2] - 1];

        // 右手座標なので反転
        position.x *= -1.0f;
        normal.x *= -1.0f;
        texcoord.y = 1.0f - texcoord.y;

        triangle[faceVertex] = {position, texcoord, normal};
      }

      // 頂点を逆順で登録することで、回り順を逆にする
      modelData.vertices.push_back(triangle[2]);
      modelData.vertices.push_back(triangle[1]);
      modelData.vertices.push_back(triangle[0]);
    } else if (identifier == "mtllib") {

      // materialTemplateLibraryファイルの名前を取得
      std::string materialFilename;

      s >> materialFilename;

      // 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
      modelData.material =
          LoadMaterialTemplateFile(directoryPath, materialFilename);
    }
  }

  // 4. ModelDataを返す
  modelData_ = modelData;
}

void Model::CreateVertexData() {
  // 頂点リソースを作る
  vertexResource = dx12Core_->CreateBufferResource(sizeof(VertexData) *
                                                   modelData_.vertices.size());

  // リソースの先頭アドレスから使う
  vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();

  // 使用するリソースのサイズは頂点のサイズ
  vertexBufferView.SizeInBytes =
      UINT(sizeof(VertexData) * modelData_.vertices.size());

  // 1頂点あたりのサイズ
  vertexBufferView.StrideInBytes = sizeof(VertexData);

  vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

  std::memcpy(vertexData, modelData_.vertices.data(),
              sizeof(VertexData) * modelData_.vertices.size());
}

void Model::CreateMaterialData() {

  materialResource = dx12Core_->CreateBufferResource(sizeof(Material));

  // マテリアルにデータを書き込む
  materialData = nullptr;

  // 書き込むためのアドレスを取得
  materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));

  // 色の指定
  materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  // Lightingさせるか
  materialData->enableLighting = false;
  // UVTransform 単位行列を入れておく
  materialData->uvTransform = MakeIdentity4x4();
}