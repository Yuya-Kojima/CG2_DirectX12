#pragma once

class BaseScene;
class EngineBase;

class SceneManager {

private:
  BaseScene *scene_ = nullptr;

  BaseScene *nextScene_ = nullptr;

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
  void SetNextScene(BaseScene *scene) { nextScene_ = scene; }

private:
  SceneManager() = default;
  ~SceneManager() = default;
  SceneManager(const SceneManager &) = delete;
  SceneManager &operator=(const SceneManager &) = delete;
  SceneManager(SceneManager &&) = delete;
  SceneManager &operator=(SceneManager &&) = delete;
};
