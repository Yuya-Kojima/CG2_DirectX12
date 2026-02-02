#include "Scene/SceneManager.h"
#include "Core/EngineBase.h"
#include "Debug/Logger.h"
#include "Scene/AbstractSceneFactory.h"
#include "Scene/BaseScene.h"
#include <cassert>

SceneManager *SceneManager::GetInstance() {
  static SceneManager instance;
  return &instance;
}

void SceneManager::Initialize(EngineBase *engine) {
  engine_ = engine;
  assert(engine_);

  SpriteRenderer *spriteRenderer = engine_->GetSpriteRenderer();
  assert(spriteRenderer);

  fade_.Initialize(spriteRenderer, "resources/white1x1.png");

  transitionState_ = TransitionState::None;
}

void SceneManager::Finalize() {

  // 予約が残ってたら解放
  if (nextScene_) {
    nextScene_->Finalize();
    nextScene_.reset();
  }

  // 現在シーン解放
  if (scene_) {
    scene_->Finalize();
    scene_.reset();
  }

  fade_.Finalize();

  engine_ = nullptr;
}

void SceneManager::Update() {

  const float dt = 1.0f / 60.0f;
  fade_.Update(dt);

  // 予約が入ったらフェードアウト開始
  if (transitionState_ == TransitionState::None) {
    if (nextScene_) {
      transitionState_ = TransitionState::FadeOut;
      fade_.StartFadeOut(0.35f);
    }
  }

  // フェードアウト完了待ち
  if (transitionState_ == TransitionState::FadeOut) {
    if (fade_.IsBlack()) {
      transitionState_ = TransitionState::SwitchScene;
    }
  }

  // シーン切替
  if (transitionState_ == TransitionState::SwitchScene) {

    // 旧シーン終了
    if (scene_) {
      scene_->Finalize();
      scene_.reset();
    }

    // 次シーンへ
    scene_ = std::move(nextScene_);

    scene_->SetSceneManger(this);
    scene_->Initialize(engine_);

    // フェードイン開始
    transitionState_ = TransitionState::FadeIn;
    fade_.StartFadeIn(0.35f);
  }

  // フェードイン完了待ち
  if (transitionState_ == TransitionState::FadeIn) {
    if (fade_.IsClear()) {
      transitionState_ = TransitionState::None;
    }
  }

  //=========================
  // シーン更新
  //=========================
  if (scene_) {
    scene_->Update();
  }
}

void SceneManager::Draw() {
  if (scene_) {
    scene_->Draw();
  }

  assert(engine_);
  engine_->Begin2D();

  fade_.Draw();
}

void SceneManager::ChangeScene(const std::string &sceneName) {

  assert(sceneFactory_);

  if (transitionState_ != TransitionState::None) {
    Logger::Log("SceneManager::ChangeScene ignored: transitioning\n");
    return;
  }

  if (nextScene_) {
    Logger::Log("SceneManager::ChangeScene ignored: nextScene already set\n");
    return;
  }

  // 次シーンを生成
  nextScene_ = sceneFactory_->CreateScene(sceneName);
}
