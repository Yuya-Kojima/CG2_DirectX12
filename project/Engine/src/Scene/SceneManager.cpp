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
      fade_.StartFadeOut(transitionDurationSec_, transitionFadeType_,
                         transitionColor_, transitionEasing_);
    }
  }

  // フェードアウト完了待ち
  if (transitionState_ == TransitionState::FadeOut) {
    if (fade_.IsCovered()) {
      transitionState_ = TransitionState::BlackHold;
      holdSec_ = 0.0f;
    }
  }

  // 黒保持
  if (transitionState_ == TransitionState::BlackHold) {
    holdSec_ += dt;
    if (holdSec_ >= holdDurationSec_) {
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
    fade_.StartFadeIn(transitionDurationSec_, transitionFadeType_,
                      transitionColor_, transitionEasing_);

    transitionDurationSec_ = 0.35f;
    transitionColor_ = Vector4{0.0f, 0.0f, 0.0f, 1.0f};
    transitionFadeType_ = Fade::FadeType::Solid;
    transitionEasing_ = Fade::EasingType::EaseInOutCubic;
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

  engine_->End2D();
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

void SceneManager::SetNextTransitionFade(float durationSec, Fade::FadeType type,
                                         const Vector4 &color,
                                         Fade::EasingType easing) {
  transitionDurationSec_ = (std::max)(durationSec, 0.0001f);
  transitionFadeType_ = type;
  transitionColor_ = color;
  transitionEasing_ = easing;
}
