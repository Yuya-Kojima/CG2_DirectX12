#pragma once
#include "VertexData.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct ModelData {
  std::vector<VertexData> vertices;
};

/// <summary>
/// Objファイルを読む
/// </summary>
/// <param name="directoryPath"></param>
/// <param name="filename"></param>
/// <returns></returns>
ModelData LoadModelFile(const std::string &directoryPath,
                        const std::string &filename) {

  // 1. 中で必要となる変数の宣言
  ModelData modelData; // 構築するModelData

  std::vector<Vector4> positions; // 位置
  std::vector<Vector3> normals;   // 法線
  std::vector<Vector2> texcoords; // テクスチャ座標
  std::string line; // ファイルから読んだ一行を格納する

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
        position.y *= -1.0f;
        normal.y *= -1.0f;

        triangle[faceVertex] = {position, texcoord, normal};
      }

      // 頂点を逆順で登録することで、回り順を逆にする
      modelData.vertices.push_back(triangle[2]);
      modelData.vertices.push_back(triangle[1]);
      modelData.vertices.push_back(triangle[0]);
    }
  }

  // 4. ModelDataを返す
  return modelData;
}
