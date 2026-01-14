#include "Model/ModelManager.h"
#include "Model/Model.h"
#include "Renderer/ModelRenderer.h"
#include <cassert>

ModelManager *ModelManager::instance = nullptr;

bool ModelManager::isFinalized_ = false;

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

  if (models.contains(filePath)) {
    // 読み込み済みなら早期リターン
    return;
  }

  // モデルの生成とファイル読み込み、初期化
  std::unique_ptr<Model> model = std::make_unique<Model>();
  model->Initialize(modelRenderer, "resources", filePath);

  // モデルをmapコンテナに格納
  models.insert(std::make_pair(filePath, std::move(model)));
}

Model *ModelManager::FindModel(const std::string &filePath) {

  // 読み込み済みモデルを検索
  if (models.contains(filePath)) {
    // 読み込みモデルを戻り値としてリターン
    return models.at(filePath).get();
  }

  // ファイル名一致なし
  return nullptr;
}
