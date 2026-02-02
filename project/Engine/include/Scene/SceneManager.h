#pragma once
#include "Sprite/Fade.h"
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

  Fade fade_;

  float transitionDurationSec_ = 0.35f;
  Vector4 transitionColor_ = Vector4{0.0f, 0.0f, 0.0f, 1.0f};
  Fade::FadeType transitionFadeType_ = Fade::FadeType::Solid;

  enum class TransitionState {
    None,
    FadeOut,
    SwitchScene,
    FadeIn,
  };

  TransitionState transitionState_ = TransitionState::None;

public:
  static SceneManager *GetInstance();

  void Initialize(EngineBase *engine);

  void Finalize();

  void Update();

  void Draw();

  void ChangeScene(const std::string &sceneName);

  void SetNextTransitionFade(float durationSec, Fade::FadeType type,
                             const Vector4 &color);

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
