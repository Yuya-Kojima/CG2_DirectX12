#include "Model/ModelManager.h"
#include "Model/Model.h"
#include "Renderer/ModelRenderer.h"
#include <cassert>
#include <filesystem>
#include "Debug/Logger.h"

ModelManager *ModelManager::instance = nullptr;

bool ModelManager::isFinalized_ = false;

namespace fs = std::filesystem;

std::string ModelManager::ResolveModelPath(const std::string &input) {
  // 入力が既にパスであればそのまま返す
  if (input.find('/') != std::string::npos || input.find('\\') != std::string::npos) {
    return input;
  }

  // 検索対象のベースディレクトリを列挙（プロジェクト内の resources と MaterialTemplate も優先）
  // Application/resources を追加して、Application/resources/MaterialTemplate 配下の仮モデルも見つかるようにする
  const std::vector<fs::path> bases = {
      "Application/resources",
      "Application/resources/MaterialTemplate",
      "resources",
      "resources/materialtemplate",
  };

  for (const auto &base : bases) {
    if (!fs::exists(base))
      continue;

    for (const auto &e : fs::recursive_directory_iterator(base)) {
      if (!e.is_regular_file())
        continue;

      if (e.path().filename() == input) {
        std::string found = e.path().generic_string();
        Logger::Log(std::string("[ModelManager] ResolveModelPath: found '") + input + "' -> '" + found + "'\n");
        return found;
      }
    }
  }

  // 見つからなければ、そのまま返す
  Logger::Log(std::string("[ModelManager] ResolveModelPath: not found '") + input + "'\n");
  return input;
}

ModelManager *ModelManager::GetInstance() {

  // すでにFinalize済ならエラーを吐く
  if (isFinalized_) {
    assert(!"ModelManager::GetInstance() was called after Finalize()!");
    return nullptr;
  }

  if (instance == nullptr) {
    instance = new ModelManager();
  }

  return instance;
}

void ModelManager::Finalize() {
  // すでにFinalize済なら警告
  if (isFinalized_) {
    assert(!"ModelManager::Finalize() called twice!");
    return;
  }

  delete instance->modelRenderer;
  instance->modelRenderer = nullptr;

  delete instance;
  instance = nullptr;
  isFinalized_ = true;
}

void ModelManager::Initialize(Dx12Core *dx12Core) {

  modelRenderer = new ModelRenderer();
  modelRenderer->Initialize(dx12Core);
}

void ModelManager::LoadModel(const std::string &filePath) {

  const std::string resolved = ResolveModelPath(filePath);

  Logger::Log(std::string("[ModelManager] LoadModel request: input='") + filePath + "' -> resolved='" + resolved + "'\n");

  if (models.contains(resolved)) {
    // 読み込み済みなら早期リターン
    Logger::Log(std::string("[ModelManager] LoadModel: already loaded '") + resolved + "'\n");
    return;
  }

  // "resources/" を基準にする
  const std::string baseDir = "resources";

  // ディレクトリ部分とファイル名部分に分解
  std::string directoryPath = baseDir;
  std::string filename = resolved;

  const size_t slash = resolved.find_last_of("/\\");
  if (slash != std::string::npos) {
    directoryPath = resolved.substr(0, slash);
    filename = resolved.substr(slash + 1);
  }
  // if (slash != std::string::npos) {
  //   directoryPath = baseDir + "/" + filePath.substr(0, slash);
  //   filename = filePath.substr(slash + 1);
  // }

  std::unique_ptr<Model> model = std::make_unique<Model>();
  model->Initialize(modelRenderer, directoryPath, filename);
  models.insert(std::make_pair(resolved, std::move(model)));
  Logger::Log(std::string("[ModelManager] LoadModel: loaded '") + resolved + "' from dir='" + directoryPath + "' file='" + filename + "'\n");

  //// モデルの生成とファイル読み込み、初期化
  // std::unique_ptr<Model> model = std::make_unique<Model>();
  // model->Initialize(modelRenderer, "resources", filePath);

  //// モデルをmapコンテナに格納
  // models.insert(std::make_pair(filePath, std::move(model)));
}

Model *ModelManager::FindModel(const std::string &filePath) {

  const std::string resolved = ResolveModelPath(filePath);

  // 読み込み済みモデルを検索
  if (models.contains(resolved)) {
    // 読み込みモデルを戻り値としてリターン
    return models.at(resolved).get();
  }

  // ファイル名一致なし
  return nullptr;
}
