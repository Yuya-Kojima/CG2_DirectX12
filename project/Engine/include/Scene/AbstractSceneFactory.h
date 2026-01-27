#pragma once
#include <memory>
#include <string>

class BaseScene;

class AbstractSceneFactory {

public:
  // 仮想デストラクタ
  virtual ~AbstractSceneFactory() = default;

  // シーン生成
  virtual std::unique_ptr<BaseScene>
  CreateScene(const std::string &sceneName) = 0;
};
