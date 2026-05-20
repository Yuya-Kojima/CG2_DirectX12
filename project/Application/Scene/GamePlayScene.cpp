#include "GamePlayScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Debug/ImGuiManager.h"
#include "Input/InputKeyState.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/SceneManager.h"
#include "Sprite/Sprite.h"
#include "Texture/TextureManager.h"

#include "Renderer/PostProcess.h"
#include "Framework/ActorManager.h"
#include <imgui.h>
#include "ImGuizmo.h"
#include <string>
#include <numbers>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include "../../externals/nlohmann/json.hpp"

void GamePlayScene::Initialize(EngineBase *engine) {

  // 基底クラスの初期化 (PostProcessの初期化など)
  BaseScene::Initialize(engine);
  
  // シーン初期化時に前シーンの残留アクター（弾や古いプレイヤー）を全消去
  ActorManager::GetInstance()->Clear();
  
  // 参照をコピー
  engine_ = engine;

  //===========================
  // テクスチャファイルの読み込み
  //===========================

  //===========================
  // オーディオファイルの読み込み
  //===========================

  //===========================
  // スプライト関係の初期化
  //===========================

  //===========================
  // 3Dオブジェクト関係の初期化
  //===========================

  // デバッグカメラ
  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize({0.0f, 4.0f, -10.0f});

  // レールカメラの初期化（真っ直ぐ奥へ進むだけの自然なレールに変更）
  waypoints_ = {
      {0.0f, 4.0f, -10.0f},
      {0.0f, 4.0f, 40.0f},
      {0.0f, 4.0f, 90.0f},
      {0.0f, 4.0f, 140.0f},
      {0.0f, 4.0f, 190.0f},
      {0.0f, 4.0f, 240.0f}
  };
  railCamera_ = std::make_unique<RailCamera>();
  railCamera_->Initialize(waypoints_);
  railCamera_->SetSpeed(0.2f); // スピードも少し落として照準を合わせやすくする

#ifdef USE_IMGUI
  // エディタモードの初期状態：デバッグカメラON、自動進行OFF
  useDebugCamera_ = true;
  railCamera_->SetAutoMove(false);
#else
  // Release版の初期状態：デバッグカメラOFF、自動進行ON
  useDebugCamera_ = false;
  railCamera_->SetAutoMove(true);
#endif

  // デフォルトカメラをレールカメラに設定
  engine_->GetObject3dRenderer()->SetDefaultCamera(railCamera_.get());

  //===========================
  // SkyBoxの初期化
  //===========================
  TextureManager::GetInstance()->LoadTexture("resources/Skybox/Skybox.dds");
  skybox_ = std::make_unique<Skybox>();
  skybox_->Initialize(engine_->GetSkyboxRenderer());
  skybox_->SetTexture("resources/Skybox/Skybox.dds");
  skybox_->SetScale({100.0f, 100.0f, 100.0f});

  //===========================
  // プレイヤーの初期化
  //===========================
  ModelManager::GetInstance()->LoadModel("suzanne.obj");

  player_ = std::make_unique<Player>();
  player_->SetSpriteRenderer(engine_->GetSpriteRenderer());
  player_->SetObject3dRenderer(engine_->GetObject3dRenderer());
  player_->SetCamera(railCamera_.get());
  player_->SetInput(engine_->GetInputManager());
  
  // プレイヤーの初期化（照準やコライダーの生成など）
  player_->Initialize();
  
  auto playerModel = std::make_unique<Object3d>();
  playerModel->Initialize(engine_->GetObject3dRenderer());
  playerModel->SetModel("suzanne.obj"); // 仮の自機モデル
  playerModel->SetColor({0.0f, 0.5f, 1.0f, 1.0f});
  player_->SetModel(std::move(playerModel));

  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f},
      {0.3f, 0.0f, 0.0f},
      {0.0f, 4.0f, -10.0f},
  };

  // デバッグカメラ
  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize({0.0f, 4.0f, -10.0f});

  // モデルの読み込み

  // オブジェクトの生成と初期化

  //===========================
  // パーティクル関係の初期化
  //===========================

  // レベルデータのロード
  LoadLevel();
  // 古いハードコード生成が走らないようにtrueにしておく
  hasSpawnedDummy_ = true;
}

void GamePlayScene::Finalize() {}

void GamePlayScene::Update() {

  // Sound更新
  SoundManager::GetInstance()->Update();

  // タイトルシーンへ移行
  if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
    SceneManager::GetInstance()->ChangeScene("DEBUG");
  }

  // デバッグカメラ切り替え
  if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
    if (useDebugCamera_) {
      useDebugCamera_ = false;
    } else {
      useDebugCamera_ = true;
    }
  }

  //=======================
  // 3Dオブジェクトの更新
  //=======================

  //=======================
  // カメラの更新
  //=======================
  const ICamera *activeCamera = nullptr;

  if (useDebugCamera_) {
    debugCamera_->Update(*engine_->GetInputManager());
    activeCamera = debugCamera_->GetCamera();
  } else {
    railCamera_->Update();
    activeCamera = railCamera_.get();
  }

  // 基底クラスにも現在のアクティブカメラを教える（Gizmo描画などで使うため）
  SetActiveCamera(const_cast<ICamera*>(activeCamera));

  //=======================
  // テスト用：ダミー敵のスポーン
  //=======================
  // （エディタからのSave/Loadに移行したため削除）


  // 敵の更新
  for (auto& enemy : enemies_) {
    enemy->Update();
  }

  // LockOn用にPlayerに敵リストを渡す（毎回最新の状態を渡す）
  enemyPtrs_.clear();
  for (auto& e : enemies_) {
    enemyPtrs_.push_back(e.get());
  }
  if (player_) {
    player_->SetEnemies(enemyPtrs_);
  }

  // アクティブカメラを描画で使用する
  engine_->GetObject3dRenderer()->SetDefaultCamera(activeCamera);

  if (skybox_) {
    skybox_->SetCamera(activeCamera);
    skybox_->Update();
  }

  // プレイヤーの更新
  if (player_) {
    // プレイヤーの照準や挙動の計算には常にレールカメラを使用する（DebugCameraに追従させないため）
    player_->SetCamera(railCamera_.get());
    player_->Update();
    
    // ロックオン中は画面をグレースケールにする（課題要件＋演出効果）
    if (postProcess_) {
      if (player_->IsLockOnMode()) {
        postProcess_->SetUseGrayscale(true);
      } else {
        postProcess_->SetUseGrayscale(false);
      }
    }
  }

  // アクター群（弾など）の更新
  ActorManager::GetInstance()->Update();

#ifdef USE_IMGUI
  //=========================
  // メインメニューバー
  //=========================
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save Level")) {
        SaveLevel();
      }
      if (ImGui::MenuItem("Load Level")) {
        LoadLevel();
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("GameObject")) {
      if (ImGui::MenuItem("Add Enemy")) {
        Vector3 camPos = activeCamera->GetTranslate();
        AddEnemy({camPos.x, camPos.y + 5.0f, camPos.z + 50.0f});
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  //=========================
  // Hierarchy ウィンドウ
  //=========================
  ImGui::Begin("Hierarchy");
  ImGui::Text("Scene Objects");
  ImGui::Separator();

  if (ImGui::Selectable("Environment (PostProcess)", currentSelectType_ == EditorSelectType::Environment)) {
    currentSelectType_ = EditorSelectType::Environment;
  }
  if (ImGui::Selectable("Rail Camera", currentSelectType_ == EditorSelectType::RailCamera)) {
    currentSelectType_ = EditorSelectType::RailCamera;
  }

  ImGui::Spacing();
  ImGui::Text("Enemies");
  ImGui::Separator();

  // 敵のリストを表示
  for (size_t i = 0; i < enemies_.size(); ++i) {
    if (enemies_[i]->IsDead()) continue;

    std::string label = "Enemy " + std::to_string(i);
    bool isSelected = (currentSelectType_ == EditorSelectType::Enemy && selectedEnemyIndex_ == static_cast<int>(i));
    if (ImGui::Selectable(label.c_str(), isSelected)) {
      currentSelectType_ = EditorSelectType::Enemy;
      selectedEnemyIndex_ = static_cast<int>(i);
    }
  }
  ImGui::End();

  //=========================
  // Inspector ウィンドウ
  //=========================
  ImGui::Begin("Inspector");

  if (currentSelectType_ == EditorSelectType::Environment) {
    ImGui::Text("Environment Settings");
    ImGui::Separator();
    auto pp = SceneManager::GetInstance()->GetCurrentScenePostProcess();
    if (pp) {
      pp->DrawDebugUI("Environment", false); // インライン描画
    } else {
      ImGui::Text("No PostProcess active.");
    }
  } else if (currentSelectType_ == EditorSelectType::RailCamera) {
    ImGui::Text("Rail Camera Waypoints");
    ImGui::Separator();
    
    if (railCamera_) {
      auto& waypoints = railCamera_->GetWaypointsRef();
      for (size_t i = 0; i < waypoints.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::Text("Point %zu", i);
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
          waypoints.erase(waypoints.begin() + i);
          ImGui::PopID();
          break; // 安全のため1フレームに1つだけ削除
        }
        ImGui::DragFloat3("##Pos", &waypoints[i].x, 0.1f);
        ImGui::PopID();
      }
      if (ImGui::Button("Add Waypoint")) {
        if (!waypoints.empty()) {
          waypoints.push_back(waypoints.back() + Vector3{0, 0, 10.0f});
        } else {
          waypoints.push_back({0, 0, 0});
        }
      }
    }
  } else if (currentSelectType_ == EditorSelectType::Enemy && selectedEnemyIndex_ >= 0 && selectedEnemyIndex_ < enemies_.size()) {
    Enemy* selected = enemies_[selectedEnemyIndex_].get();
    if (!selected->IsDead()) {
      ImGui::Text("Enemy %d", selectedEnemyIndex_);
      ImGui::Separator();
      Transform& t = selected->GetTransform();
      ImGui::DragFloat3("Translate", &t.translate.x, 0.1f);
      ImGui::DragFloat3("Rotate", &t.rotate.x, 0.01f);
      ImGui::DragFloat3("Scale", &t.scale.x, 0.1f);
    } else {
      currentSelectType_ = EditorSelectType::None; // 死んだら選択解除
    }
  } else {
    ImGui::Text("No object selected.");
  }
  ImGui::End();

  //=========================
  // Timeline & Sequencer UI
  //=========================
  ImGui::Begin("Timeline & Sequencer");
  
  bool isAutoMove = railCamera_->GetAutoMove();
  if (ImGui::Checkbox("Auto Play RailCamera", &isAutoMove)) {
    railCamera_->SetAutoMove(isAutoMove);
  }

  ImGui::SameLine();
  ImGui::Checkbox("Debug Camera [P]", &useDebugCamera_);

  float currentT = railCamera_->GetT();
  float maxT = static_cast<float>(railCamera_->GetWaypoints().size() - 1);
  if (maxT < 0.0f) maxT = 0.0f;

  if (ImGui::SliderFloat("Time (t)", &currentT, 0.0f, maxT)) {
    railCamera_->SetT(currentT);
    // スライダーを直接動かしている間は自動再生をオフにする
    railCamera_->SetAutoMove(false);
  }

  ImGui::End();

  // レールのデバッグ描画（線でウェイポイントを結ぶ）
  const auto& waypoints = railCamera_->GetWaypoints();
  if (waypoints.size() > 1) {
    for (size_t i = 0; i < waypoints.size() - 1; ++i) {
      engine_->GetLineRenderer()->DrawLine(waypoints[i], waypoints[i+1], {0.0f, 1.0f, 1.0f, 1.0f}); // シアン色の線
    }
    // 現在のカメラ位置に赤い目印をつける（短い線でクロスを描く等）
    Vector3 camPos = railCamera_->CalcPosition(currentT);
    engine_->GetLineRenderer()->DrawLine({camPos.x - 2, camPos.y, camPos.z}, {camPos.x + 2, camPos.y, camPos.z}, {1.0f, 0.0f, 0.0f, 1.0f});
    engine_->GetLineRenderer()->DrawLine({camPos.x, camPos.y - 2, camPos.z}, {camPos.x, camPos.y + 2, camPos.z}, {1.0f, 0.0f, 0.0f, 1.0f});
    engine_->GetLineRenderer()->DrawLine({camPos.x, camPos.y, camPos.z - 2}, {camPos.x, camPos.y, camPos.z + 2}, {1.0f, 0.0f, 0.0f, 1.0f});
  }

#endif
}

void GamePlayScene::DrawEditorUI() {
#ifdef USE_IMGUI
  if (currentSelectType_ == EditorSelectType::Enemy && selectedEnemyIndex_ >= 0 && selectedEnemyIndex_ < enemies_.size()) {
    Enemy* selected = enemies_[selectedEnemyIndex_].get();
    if (!selected->IsDead()) {
      // Game Viewの現在の領域を取得
      ImVec2 windowPos = ImGui::GetCursorScreenPos();
      ImVec2 windowSize = ImGui::GetContentRegionAvail();
      // 上にImageが描画されたあとだとCursorが下に移動しているので、上に戻すか描画前に取る必要がある。
      // Wait, DrawEditorUI() is called AFTER ImGui::Image() in Game.cpp.
      // So GetCursorScreenPos() is at the bottom of the image!
      // Image size was `renderWidth, renderHeight`.
      // We can use ImGui::GetWindowPos() and ImGui::GetWindowSize() or ImGui::GetItemRectMin(), ImGui::GetItemRectMax()!
      ImVec2 vMin = ImGui::GetItemRectMin();
      ImVec2 vMax = ImGui::GetItemRectMax();

      ImGuizmo::SetDrawlist();
      ImGuizmo::SetRect(vMin.x, vMin.y, vMax.x - vMin.x, vMax.y - vMin.y);

      // アクティブなカメラの取得
      ICamera* camera = GetActiveCamera();
      if (!camera) return;

      Matrix4x4 viewMatrix = camera->GetViewMatrix();
      Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();

      Transform& t = selected->GetTransform();
      Matrix4x4 worldMatrix = MakeAffineMatrix(t.scale, t.rotate, t.translate);

      static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
      static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

      // 右クリック中はカメラ操作中なので、Gizmoのショートカットを無効にする
      if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        if (ImGui::IsKeyPressed(ImGuiKey_W))
          mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E))
          mCurrentGizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
          mCurrentGizmoOperation = ImGuizmo::SCALE;
      }

      // Gizmo操作用のUIウィンドウ
      ImGui::Begin("Gizmo Tools");
      if (ImGui::RadioButton("Translate [W]", mCurrentGizmoOperation == ImGuizmo::TRANSLATE)) {
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Rotate [E]", mCurrentGizmoOperation == ImGuizmo::ROTATE)) {
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Scale [R]", mCurrentGizmoOperation == ImGuizmo::SCALE)) {
        mCurrentGizmoOperation = ImGuizmo::SCALE;
      }

      if (mCurrentGizmoOperation != ImGuizmo::SCALE) {
        if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL)) {
          mCurrentGizmoMode = ImGuizmo::LOCAL;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD)) {
          mCurrentGizmoMode = ImGuizmo::WORLD;
        }
      }
      ImGui::End();

      ImGuizmo::Manipulate(&viewMatrix.m[0][0], &projectionMatrix.m[0][0], mCurrentGizmoOperation, mCurrentGizmoMode, &worldMatrix.m[0][0]);

      if (ImGuizmo::IsUsing()) {
        float matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents(&worldMatrix.m[0][0], matrixTranslation, matrixRotation, matrixScale);
        t.translate = {matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]};
        // ImGuizmo returns rotation in degrees
        t.rotate = {
            matrixRotation[0] * (std::numbers::pi_v<float> / 180.0f),
            matrixRotation[1] * (std::numbers::pi_v<float> / 180.0f),
            matrixRotation[2] * (std::numbers::pi_v<float> / 180.0f)};
        t.scale = {matrixScale[0], matrixScale[1], matrixScale[2]};
      }
    }
  }
#endif
}

void GamePlayScene::Draw() {
  Draw3D();
}

void GamePlayScene::Draw3D() {
  engine_->Begin3D();

  if (skybox_) {
    engine_->GetSkyboxRenderer()->Begin();
    skybox_->Draw();
    
    // Skybox描画後に通常3Dへ戻す
    engine_->GetObject3dRenderer()->Begin();
  }

  // 敵の描画
  for (auto& enemy : enemies_) {
    if (!enemy->IsDead()) {
      enemy->Draw3D();
    }
  }

  if (player_) {
    player_->Draw3D();
  }

  // アクター群（弾など）の描画
  ActorManager::GetInstance()->Draw3D();

#ifdef USE_IMGUI
  // デバッグ用の線を描画
  const ICamera* activeCamera = camera_.get();
  if (useDebugCamera_) {
    activeCamera = debugCamera_->GetCamera();
  } else if (railCamera_) {
    activeCamera = railCamera_.get();
  }
  engine_->GetLineRenderer()->Render(activeCamera->GetViewProjectionMatrix());
#endif

  engine_->End3D();
}

void GamePlayScene::Draw2D() {
  if (player_) {
    player_->Draw2D();
  }
}

void GamePlayScene::SaveLevel() {
  nlohmann::json root;
  nlohmann::json enemiesArray = nlohmann::json::array();
  
  for (auto& enemy : enemies_) {
    if (enemy->IsDead()) continue;
    Transform& t = enemy->GetTransform();
    nlohmann::json eJson;
    eJson["translation"] = {t.translate.x, t.translate.y, t.translate.z};
    eJson["rotation"] = {t.rotate.x, t.rotate.y, t.rotate.z};
    eJson["scale"] = {t.scale.x, t.scale.y, t.scale.z};
    enemiesArray.push_back(eJson);
  }
  root["enemies"] = enemiesArray;

  // 保存先フォルダ（resources/levels）が存在しない場合は作成する
  std::filesystem::create_directories("resources/levels");

  std::ofstream file("resources/levels/level_editor.json");
  if (file.is_open()) {
    file << std::setw(4) << root << std::endl;
  }
}

void GamePlayScene::LoadLevel() {
  std::ifstream file("resources/levels/level_editor.json");
  if (!file.is_open()) return;

  nlohmann::json root;
  file >> root;

  // 古い敵をクリア
  enemies_.clear();
  selectedEnemyIndex_ = -1;

  if (root.contains("enemies")) {
    for (auto& eJson : root["enemies"]) {
      Vector3 t = {eJson["translation"][0], eJson["translation"][1], eJson["translation"][2]};
      Vector3 r = {eJson["rotation"][0], eJson["rotation"][1], eJson["rotation"][2]};
      Vector3 s = {eJson["scale"][0], eJson["scale"][1], eJson["scale"][2]};
      
      auto dummy = std::make_unique<Enemy>();
      dummy->Initialize();
      
      auto dummyModel = std::make_unique<Object3d>();
      dummyModel->Initialize(engine_->GetObject3dRenderer());
      dummyModel->SetModel("suzanne.obj");
      dummyModel->SetColor({1.0f, 0.2f, 0.2f, 1.0f});
      dummy->SetModel(std::move(dummyModel));
      
      dummy->GetTransform().translate = t;
      dummy->GetTransform().rotate = r;
      dummy->GetTransform().scale = s;
      
      enemies_.push_back(std::move(dummy));
    }
  }
}

void GamePlayScene::AddEnemy(const Vector3& position) {
  auto dummy = std::make_unique<Enemy>();
  dummy->Initialize();

  auto dummyModel = std::make_unique<Object3d>();
  dummyModel->Initialize(engine_->GetObject3dRenderer());
  dummyModel->SetModel("suzanne.obj");
  dummyModel->SetColor({1.0f, 0.2f, 0.2f, 1.0f});
  dummy->SetModel(std::move(dummyModel));

  dummy->GetTransform().translate = position;
  dummy->GetTransform().scale = {3.0f, 3.0f, 3.0f};

  enemies_.push_back(std::move(dummy));
}