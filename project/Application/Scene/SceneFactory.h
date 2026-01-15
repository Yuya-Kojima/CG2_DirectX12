#pragma once
#include "Scene/AbstractSceneFactory.h"

class SceneFactory : public AbstractSceneFactory {

public:
  /// <summary>
  /// シーン生成
  /// </summary>
  /// <param name="sceneName"></param>
  /// <returns></returns>
  BaseScene *CreateScene(const std::string &sceneName) override;
};
