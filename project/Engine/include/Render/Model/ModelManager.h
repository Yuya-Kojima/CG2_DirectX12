#pragma once
#include <map>
#include <memory>
#include <string>

class Model;

class ModelRenderer;

class Dx12Core;

class ModelManager {

public:
  static ModelManager *GetInstance();

  static void Finalize();

  void Initialize(Dx12Core *dx12Core);

private:
  static ModelManager *instance;

  static bool isFinalized_;

  ModelManager() = default;
  ~ModelManager() = default;
  ModelManager(ModelManager &) = delete;
  ModelManager &operator=(ModelManager) = delete;

private:
  // モデルデータ
  std::map<std::string, std::unique_ptr<Model>> models;

  ModelRenderer *modelRenderer = nullptr;

public:
  /// <summary>
  /// モデルファイルの読み込み
  /// </summary>
  /// <param name="filePath">ファイルパス</param>
  void LoadModel(const std::string &filePath);

  /// <summary>
  /// モデルの検索
  /// </summary>
  /// <param name="filePath"></param>
  Model *FindModel(const std::string &filePath);

  static std::string ResolveModelPath(const std::string &input);
};
