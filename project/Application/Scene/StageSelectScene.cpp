#include "StageSelectScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Input/InputKeyState.h"
#include "Model/ModelManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/SceneManager.h"
#include "Texture/TextureManager.h"
#include "Framework/GameManager.h"

#ifdef USE_IMGUI
#include "Debug/ImGuiManager.h"
#endif

void StageSelectScene::Initialize(EngineBase *engine) {

  // 参照をコピー
  engine_ = engine;

  // カメラの生成と初期化
  camera_ = std::make_unique<GameCamera>();
  camera_->SetRotate({0.3f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 4.0f, -10.0f});

  // デバッグカメラ
  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize({0.0f, 4.0f, -10.0f});

  // デフォルトカメラのセット
  engine_->GetObject3dRenderer()->SetDefaultCamera(camera_.get());
}

void StageSelectScene::Finalize() {}

void StageSelectScene::Update() {

  // Sound更新
  SoundManager::GetInstance()->Update();

  // デバッグカメラ切り替え
  if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
    if (useDebugCamera_) {
      useDebugCamera_ = false;
    } else {
      useDebugCamera_ = true;
    }
  }

#ifdef USE_IMGUI
  // ImGuiを使った仮のステージ選択UI
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 center = ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + viewport->Size.y * 0.5f);
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::Begin("Stage Select", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
  
  ImGui::Text("Select a Stage:");
  ImGui::Separator();

  if (ImGui::Button("Stage 1 (level_editor.json)", ImVec2(300, 50))) {
    GameManager::GetInstance()->SetCurrentLevel("level_editor.json");
    SceneManager::GetInstance()->SetNextTransitionFade(0.5f);
    SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
  }

  // 今後ステージが増えたらここに追加
  if (ImGui::Button("Stage 2 (TBD)", ImVec2(300, 50))) {
    GameManager::GetInstance()->SetCurrentLevel("level_editor.json"); // とりあえず同じ
    SceneManager::GetInstance()->SetNextTransitionFade(0.5f);
    SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
  }

  ImGui::Separator();
  if (ImGui::Button("Back to Title", ImVec2(300, 30))) {
    SceneManager::GetInstance()->SetNextTransitionFade(0.5f);
    SceneManager::GetInstance()->ChangeScene("TITLE");
  }

  ImGui::End();
#endif

  //=======================
  // カメラの更新
  //=======================
  const ICamera *activeCamera = nullptr;

  if (useDebugCamera_) {
    debugCamera_->Update(*engine_->GetInputManager());
    activeCamera = debugCamera_->GetCamera();
  } else {
    camera_->Update();
    activeCamera = camera_.get();
  }

  // アクティブカメラを描画で使用する
  engine_->GetObject3dRenderer()->SetDefaultCamera(activeCamera);
}

void StageSelectScene::Draw() {
  Draw3D();
}

void StageSelectScene::Draw3D() {
  engine_->Begin3D();

  // ここから下で3DオブジェクトのDrawを呼ぶ
}

void StageSelectScene::Draw2D() {
  // ここから下で2DオブジェクトのDrawを呼ぶ
}
