#pragma once
#include <memory>
#include <string>

class BaseScene;
class EngineBase;
class AbstractSceneFactory;

class SceneManager {

private:
  std::unique_ptr<BaseScene> scene_ = nullptr;

  std::unique_ptr<BaseScene> nextScene_ = nullptr;

  EngineBase *engine_ = nullptr;

public:
  static SceneManager *GetInstance();

  void Initialize(EngineBase *engine);

  void Finalize();

  void Update();

  void Draw();

  //~SceneManager();

  /// <summary>
  /// 次のシーンを予約
  /// </summary>
  /// <param name="scene"></param>
  // void SetNextScene(BaseScene *scene) { nextScene_ = scene; }

  void ChangeScene(const std::string &sceneName);

private:
  SceneManager() = default;
  ~SceneManager() = default;
  SceneManager(const SceneManager &) = delete;
  SceneManager &operator=(const SceneManager &) = delete;
  SceneManager(SceneManager &&) = delete;
  SceneManager &operator=(SceneManager &&) = delete;

private:
  // シーンファクトリー（借りてくる）
  AbstractSceneFactory *sceneFactory_ = nullptr;

public:
  void SetSceneFactory(AbstractSceneFactory *sceneFactory) {
    sceneFactory_ = sceneFactory;
  }
};
