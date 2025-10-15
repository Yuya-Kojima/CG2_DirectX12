#pragma once
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

//struct MaterialData {
//  std::string textureFilePath;
//};
//
//MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
//                                      const std::string &filename) {
//
//  // 1. 中で必要となる変数の宣言
//  MaterialData materialData; // 構築するMaterialData
//  std::string line;          // ファイルから読んだ一行を格納する
//
//  // 2. ファイルを開く
//  std::ifstream file(directoryPath + "/" + filename);
//  assert(file.is_open()); // とりあえず開けなかったら止める
//
//  // 3. 実際にファイルを読み、MaterialDataを構築していく
//  while (std::getline(file, line)) {
//
//    std::string identifier;
//    std::istringstream s(line);
//
//    s >> identifier;
//
//    // identifierに応じた処理
//    if (identifier == "map_Kd") {
//      std::string textureFilename;
//      s >> textureFilename;
//      // 連結してファイルパスにする
//      materialData.textureFilePath = directoryPath + "/" + textureFilename;
//    }
//  }
//
//  // 4. MaterialDataを返す
//  return materialData;
//}