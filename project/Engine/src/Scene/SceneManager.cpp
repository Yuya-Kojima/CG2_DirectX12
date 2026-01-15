#include "Scene/SceneManager.h"
#include "Core/EngineBase.h"
#include "Scene/BaseScene.h"

SceneManager *SceneManager::GetInstance() {
  static SceneManager instance;
  return &instance;
}

void SceneManager::Initialize(EngineBase *engine) { engine_ = engine; }

void SceneManager::Finalize() {

  // 予約が残ってたら解放
  if (nextScene_) {
    nextScene_->Finalize();
    delete nextScene_;
    nextScene_ = nullptr;
  }

  // 現在シーン解放
  if (scene_) {
    scene_->Finalize();
    delete scene_;
    scene_ = nullptr;
  }

  engine_ = nullptr;
}

void SceneManager::Update() {

  // 予約確認
  if (nextScene_) {

    // 旧シーンの終了
    if (scene_) {
      scene_->Finalize();
      delete scene_;
    }

    // シーン切り替え
    scene_ = nextScene_;
    nextScene_ = nullptr;

    scene_->SetSceneManger(this);

    scene_->Initialize(engine_);
  }

  // シーンの更新
  if (scene_) {
    scene_->Update();
  }
}

void SceneManager::Draw() {
  if (scene_) {
    scene_->Draw();
  }
}

//SceneManager::~SceneManager() {
//  scene_->Finalize();
//  delete scene_;
//  scene_ = nullptr;
//}
