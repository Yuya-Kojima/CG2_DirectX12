#include "SceneFactory.h"
#include "ClearScene.h"
#include "DebugScene.h"
#include "GamePlayScene.h"
#include "StageSelectScene.h"
#include "TitleScene.h"

std::unique_ptr<BaseScene>
SceneFactory::CreateScene(const std::string &sceneName) {

  // 次のシーンを生成
  BaseScene *newScene = nullptr;

  if (sceneName == "TITLE") {
    return std::make_unique<TitleScene>();
  } else if (sceneName == "GAMEPLAY") {
    return std::make_unique<GamePlayScene>();
  } else if (sceneName == "DEBUG") {
    return std::make_unique<DebugScene>();
  } else if (sceneName == "STAGESELECT") {
    return std::make_unique<StageSelectScene>();
  } else if (sceneName == "CLEAR") {
    return std::make_unique<ClearScene>();
  }

  assert(false && "Unknown sceneName");
  return nullptr;
}
