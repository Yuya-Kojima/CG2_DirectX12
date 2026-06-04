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
#include "Framework/PrefabManager.h"
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
  PrefabManager::GetInstance()->Initialize(engine->GetObject3dRenderer());
  
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
  debugCamera_->Initialize({0.0f, 10.0f, -30.0f});

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
  player_->GetTransform().translate = {0.0f, 4.0f, 0.0f}; // レールカメラの初期位置に合わせる
  
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
  debugCamera_->Initialize({0.0f, 10.0f, -30.0f});

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
  bool shouldUpdateLogic = (isPlayMode_ && !isPaused_) || doStep_;

  if (doStep_) {
    doStep_ = false; // Reset step flag after one frame
  }

  // ポストエフェクトの更新
  //=======================

  //=======================
  // カメラの更新
  //=======================
  const ICamera *activeCamera = nullptr;

  if (useDebugCamera_) {
    debugCamera_->Update(*engine_->GetInputManager());
    activeCamera = debugCamera_->GetCamera();
  } else {
    if (!shouldUpdateLogic) {
      bool autoMoveCache = railCamera_->GetAutoMove();
      railCamera_->SetAutoMove(false);
      railCamera_->Update();
      railCamera_->SetAutoMove(autoMoveCache);
    } else {
      railCamera_->Update();
    }
    activeCamera = railCamera_.get();
    
    // スポナーロジック
    if (railCamera_) {
      float t = railCamera_->GetT();
      for (auto& ev : spawnEvents_) {
        if (t < ev.spawnTime) {
          ev.hasSpawned = false; // シークバック時にフラグをリセット
        } else if (shouldUpdateLogic && !ev.hasSpawned && t >= ev.spawnTime) {
          // スポーン (カメラの現在位置からの相対座標で計算)
          Matrix4x4 viewMatrix = railCamera_->GetViewMatrix();
          Matrix4x4 cameraWorld = Inverse(viewMatrix);
          Vector3 cameraPos = {cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2]};
          Vector3 cameraRight = {cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2]};
          Vector3 cameraUp = {cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2]};
          Vector3 cameraForward = {cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2]};

          Vector3 spawnWorldPos = cameraPos 
                                + Vector3{cameraRight.x * ev.spawnOffset.x, cameraRight.y * ev.spawnOffset.x, cameraRight.z * ev.spawnOffset.x}
                                + Vector3{cameraUp.x * ev.spawnOffset.y, cameraUp.y * ev.spawnOffset.y, cameraUp.z * ev.spawnOffset.y}
                                + Vector3{cameraForward.x * ev.spawnOffset.z, cameraForward.y * ev.spawnOffset.z, cameraForward.z * ev.spawnOffset.z};

          auto newEnemy = PrefabManager::GetInstance()->InstantiateEnemy(ev.prefabName, Transform{{3.0f, 3.0f, 3.0f}, {0,0,0}, spawnWorldPos});
          
          // 進行方向をカメラの逆（画面奥から手前）に設定
          newEnemy->SetMoveDirection({-cameraForward.x, -cameraForward.y, -cameraForward.z});

          if (isPlayMode_) {
            runtimeEnemies_.push_back(std::move(newEnemy));
          } else {
            editorEnemies_.push_back(std::move(newEnemy));
          }
          ev.hasSpawned = true;
        }
      }
    }
  }

  // 基底クラスにも現在のアクティブカメラを教える（Gizmo描画などで使うため）
  SetActiveCamera(const_cast<ICamera*>(activeCamera));

  //=======================
  // テスト用：ダミー敵のスポーン
  //=======================
  // （エディタからのSave/Loadに移行したため削除）


  // 敵の更新
  auto& activeEnemies = isPlayMode_ ? runtimeEnemies_ : editorEnemies_;
  for (auto& enemy : activeEnemies) {
    if (shouldUpdateLogic) {
      enemy->Update();
    } else {
      enemy->UpdateTransform();
    }
  }
  for (auto& obj : sceneObjects_) {
    obj->Update();
  }

  // LockOn用にPlayerに敵リストを渡す（毎回最新の状態を渡す）
  enemyPtrs_.clear();
  for (auto& e : activeEnemies) {
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
    if (shouldUpdateLogic) {
      player_->Update();
    } else {
      player_->UpdateTransform();
    }
    
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
        Vector3 pPos = player_ ? player_->GetTransform().translate : Vector3{0,0,0};
        AddEnemy({pPos.x, pPos.y + 2.0f, pPos.z + 10.0f});
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
  bool isRailCameraOpen = ImGui::TreeNodeEx("Rail Camera", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | (currentSelectType_ == EditorSelectType::RailCamera && selectedWaypointIndex_ == -1 ? ImGuiTreeNodeFlags_Selected : 0));
  if (ImGui::IsItemClicked()) {
    currentSelectType_ = EditorSelectType::RailCamera;
    selectedWaypointIndex_ = -1;
  }
  if (isRailCameraOpen) {
    if (railCamera_) {
      auto& waypoints = railCamera_->GetWaypointsRef();
      for (size_t i = 0; i < waypoints.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        std::string wpLabel = "Waypoint " + std::to_string(i);
        bool wpSelected = (currentSelectType_ == EditorSelectType::RailCamera && selectedWaypointIndex_ == static_cast<int>(i));
        if (ImGui::Selectable(wpLabel.c_str(), wpSelected)) {
          currentSelectType_ = EditorSelectType::RailCamera;
          selectedWaypointIndex_ = static_cast<int>(i);
        }
        ImGui::PopID();
      }
    }
    ImGui::TreePop();
  }

  ImGui::Spacing();
  ImGui::Text("Enemies");
  ImGui::Separator();

  // 敵のリストを表示
  for (size_t i = 0; i < editorEnemies_.size(); ++i) {
    if (editorEnemies_[i]->IsDead()) continue;

    std::string label = "Enemy " + std::to_string(i);
    bool isSelected = (currentSelectType_ == EditorSelectType::Enemy && selectedEnemyIndex_ == static_cast<int>(i));
    if (ImGui::Selectable(label.c_str(), isSelected)) {
      currentSelectType_ = EditorSelectType::Enemy;
      selectedEnemyIndex_ = static_cast<int>(i);
    }
  }

  ImGui::Spacing();
  ImGui::Text("Scene Objects (Dynamic)");
  ImGui::Separator();
  for (size_t i = 0; i < sceneObjects_.size(); ++i) {
    std::string label = "Object " + std::to_string(i) + " (" + sceneObjects_[i]->GetModelPath() + ")";
    bool isSelected = (currentSelectType_ == EditorSelectType::SceneObject && selectedSceneObjectIndex_ == static_cast<int>(i));
    if (ImGui::Selectable(label.c_str(), isSelected)) {
      currentSelectType_ = EditorSelectType::SceneObject;
      selectedSceneObjectIndex_ = static_cast<int>(i);
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
  } else if (currentSelectType_ == EditorSelectType::Enemy && selectedEnemyIndex_ >= 0 && selectedEnemyIndex_ < editorEnemies_.size()) {
    Enemy* selected = editorEnemies_[selectedEnemyIndex_].get();
    if (!selected->IsDead()) {
      ImGui::Text("Enemy %d", selectedEnemyIndex_);
      ImGui::Separator();
      
      char tagBuf[128] = {0};
      snprintf(tagBuf, sizeof(tagBuf), "%s", selected->GetTag().c_str());
      if (ImGui::InputText("Tag", tagBuf, sizeof(tagBuf))) {
        selected->SetTag(tagBuf);
      }
      
      ImGui::Spacing();
      static char prefabNameBuf[64] = "ZakoEnemy";
      ImGui::InputText("Prefab Name", prefabNameBuf, sizeof(prefabNameBuf));
      if (ImGui::Button("Save as Prefab")) {
        // PrefabManager は後で include して実装します
        PrefabManager::GetInstance()->SavePrefab(prefabNameBuf, selected);
      }
      
      ImGui::Separator();
      
      Transform& t = selected->GetTransform();
      ImGui::DragFloat3("Translate", &t.translate.x, 0.1f);
      ImGui::DragFloat3("Rotate", &t.rotate.x, 0.01f);
      ImGui::DragFloat3("Scale", &t.scale.x, 0.1f);
    } else {
      currentSelectType_ = EditorSelectType::None; // 死んだら選択解除
    }
  } else if (currentSelectType_ == EditorSelectType::SceneObject && selectedSceneObjectIndex_ >= 0 && selectedSceneObjectIndex_ < sceneObjects_.size()) {
    Object3d* selected = sceneObjects_[selectedSceneObjectIndex_].get();
    ImGui::Text("Scene Object %d", selectedSceneObjectIndex_);
    ImGui::TextDisabled("%s", selected->GetModelPath().c_str());
    ImGui::Separator();
    
    char tagBuf[128] = {0};
    snprintf(tagBuf, sizeof(tagBuf), "%s", selected->tag_.c_str());
    if (ImGui::InputText("Tag", tagBuf, sizeof(tagBuf))) {
      selected->tag_ = tagBuf;
    }
    ImGui::Separator();
    
    Vector3 t = selected->GetTranslation();
    Vector3 r = selected->GetRotation();
    Vector3 s = selected->GetScale();
    if (ImGui::DragFloat3("Translate", &t.x, 0.1f)) selected->SetTranslation(t);
    if (ImGui::DragFloat3("Rotate", &r.x, 0.01f)) selected->SetRotation(r);
    if (ImGui::DragFloat3("Scale", &s.x, 0.1f)) selected->SetScale(s);
  } else {
    ImGui::Text("No object selected.");
  }
  ImGui::End();

  //=========================
  // Timeline & Sequencer UI
  //=========================
  ImGui::Begin("Timeline & Sequencer");
  
  ImGui::Checkbox("Debug Camera [P]", &useDebugCamera_);

  float currentT = railCamera_->GetT();
  float maxT = static_cast<float>(railCamera_->GetWaypoints().size() - 1);
  if (maxT < 0.0f) maxT = 0.0f;
  // Playモード中はスライダーの操作を無効化（表示のみ）
  if (isPlayMode_) {
    ImGui::BeginDisabled();
  }
  
  if (ImGui::SliderFloat("Rail Progress (t)", &currentT, 0.0f, maxT)) {
    railCamera_->SetT(currentT);

    // レールカメラの位置を即座に更新してプレイヤーを追従させる
    bool autoMoveCache = railCamera_->GetAutoMove();
    railCamera_->SetAutoMove(false);
    railCamera_->Update();
    railCamera_->SetAutoMove(autoMoveCache);
    
    // スライダーを直接動かした時は自動再生をオフにする（ポーズ状態にする）
    isPaused_ = true;
    if (player_) {
      player_->SetCamera(railCamera_.get());
      player_->ForceSnapToCamera();
    }
  }

  if (isPlayMode_) {
    ImGui::EndDisabled();
  }

  ImGui::Separator();
  ImGui::Text("Spawn Events");
  static char spawnPrefabBuf[64] = "ZakoEnemy";
  ImGui::InputText("Prefab Name", spawnPrefabBuf, sizeof(spawnPrefabBuf));
  ImGui::SameLine();
  if (ImGui::Button("Add Spawn Event at Current Time")) {
      SpawnEvent ev;
      ev.spawnTime = currentT;
      ev.prefabName = spawnPrefabBuf;
      ev.spawnOffset = {0.0f, 0.0f, 50.0f}; // デフォルトは画面正面の50m奥
      spawnEvents_.push_back(ev);
  }
  
  ImGui::BeginChild("SpawnEventsList", ImVec2(0, 150), true);
  for (size_t i = 0; i < spawnEvents_.size(); ++i) {
      ImGui::PushID(static_cast<int>(i));
      auto& ev = spawnEvents_[i];
      ImGui::Text("Event %zu: ", i);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("Time", &ev.spawnTime, 0.01f);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(150);
      char pNameBuf[64];
      snprintf(pNameBuf, sizeof(pNameBuf), "%s", ev.prefabName.c_str());
      if (ImGui::InputText("Prefab", pNameBuf, sizeof(pNameBuf))) {
          ev.prefabName = pNameBuf;
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(150);
      ImGui::DragFloat3("Offset", &ev.spawnOffset.x, 0.1f);
      ImGui::SameLine();
      if (ImGui::Button("Delete")) {
          spawnEvents_.erase(spawnEvents_.begin() + i);
          ImGui::PopID();
          break; // イテレータが壊れるのでループを抜ける
      }
      ImGui::PopID();
  }
  ImGui::EndChild();

  ImGui::End();

  // レールのデバッグ描画（スプライン曲線をサンプリングして描画）
  const auto& waypoints = railCamera_->GetWaypoints();
  if (waypoints.size() > 1) {
    float maxPathT = static_cast<float>(waypoints.size() - 1);
    const float step = 0.05f; // サンプリング間隔
    Vector3 prevPos = railCamera_->CalcPosition(0.0f);
    
    // スプライン曲線を滑らかに描画する
    for (float t = step; t <= maxPathT; t += step) {
      Vector3 currentPos = railCamera_->CalcPosition(t);
      engine_->GetLineRenderer()->DrawLine(prevPos, currentPos, {0.0f, 1.0f, 1.0f, 1.0f}); // シアン色の線
      prevPos = currentPos;
    }
    // 端数調整（最後の終点まで確実に結ぶ）
    engine_->GetLineRenderer()->DrawLine(prevPos, railCamera_->CalcPosition(maxPathT), {0.0f, 1.0f, 1.0f, 1.0f});
    
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
  // ======================================
  // 共通の Gizmo UI および状態管理
  // ======================================
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

  // 共通の描画領域取得 (ImGui::Image の直後に呼ばれる前提なので、直前のItemRectがGame Viewの画像サイズになる)
  ImVec2 vMin = ImGui::GetItemRectMin();
  ImVec2 vMax = ImGui::GetItemRectMax();

  // Gizmo用UIを Main Toolbar に追記
  ImGui::Begin("Main Toolbar");
  
  // ===================
  // Play / Stop Buttons
  // ===================
  if (!isPlayMode_) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f)); // 緑っぽく
    if (ImGui::Button(" [ PLAY ] ", ImVec2(80, 0))) {
      SaveLevel("level_editor_temp.json");
      isPlayMode_ = true;
      isPaused_ = false;
      useDebugCamera_ = false; // Play時は強制的にゲームカメラに戻す
      if (railCamera_) {
        railCamera_->SetAutoMove(true);
        playStartT_ = railCamera_->GetT(); // Playを開始した時点の時間を記憶
      }
      
      // Data Separation: editorEnemies_ を複製して runtimeEnemies_ を作成
      runtimeEnemies_.clear();
      for (auto& e : editorEnemies_) {
        if (e->IsDead()) continue;
        auto clone = std::make_unique<Enemy>();
        clone->Initialize();
        
        // 仮のモデルを割り当て（本来はPrefabManagerを使うべきだが既存コードに合わせる）
        auto cloneModel = std::make_unique<Object3d>();
        cloneModel->Initialize(engine_->GetObject3dRenderer());
        cloneModel->SetModel("suzanne.obj");
        cloneModel->SetColor(e->GetBaseColor());
        clone->SetModel(std::move(cloneModel));
        clone->SetBaseColor(e->GetBaseColor());
        
        clone->GetTransform() = e->GetTransform();
        clone->SetTag(e->GetTag());
        runtimeEnemies_.push_back(std::move(clone));
      }
    }
    ImGui::PopStyleColor();
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // 赤っぽく
    if (ImGui::Button(" [ STOP ] ", ImVec2(80, 0))) {
      isPlayMode_ = false;
      isPaused_ = false;
      useDebugCamera_ = true; // Stop時はエディタカメラ（DebugCamera）に戻す
      LoadLevel("level_editor_temp.json");
      if (railCamera_) {
        railCamera_->SetT(playStartT_); // STOP時はPlay開始時の時間に戻す（Play From Hereの動作）
        // レールカメラの位置を即座に更新してプレイヤーを追従させる
        bool autoMoveCache = railCamera_->GetAutoMove();
        railCamera_->SetAutoMove(false);
        railCamera_->Update();
        railCamera_->SetAutoMove(autoMoveCache);
        for (auto& ev : spawnEvents_) {
          ev.hasSpawned = false;
        }
      }
      if (player_) {
        player_->SetCamera(railCamera_.get());
        player_->ForceSnapToCamera();
      }
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (isPaused_) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f)); // 緑
      if (ImGui::Button(" [ RESUME ] ", ImVec2(80, 0))) {
        isPaused_ = false;
      }
      ImGui::PopStyleColor();
      
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.2f, 1.0f)); // 黄色
      if (ImGui::Button(" [ STEP ] ", ImVec2(80, 0))) {
        doStep_ = true;
      }
      ImGui::PopStyleColor();
    } else {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.2f, 1.0f)); // 黄色
      if (ImGui::Button(" [ PAUSE ] ", ImVec2(80, 0))) {
        isPaused_ = true;
      }
      ImGui::PopStyleColor();
    }
  }
  
  ImGui::SameLine();
  ImGui::Text("   |   Gizmo:");
  ImGui::SameLine();
  
  // Waypoint選択時はTranslateモードに固定する
  bool isWaypointMode = (currentSelectType_ == EditorSelectType::RailCamera && selectedWaypointIndex_ >= 0);
  if (isWaypointMode) {
    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::TextDisabled("Rotate [E]");
    ImGui::SameLine();
    ImGui::TextDisabled("Scale [R]");
  } else {
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
  }

  if (mCurrentGizmoOperation != ImGuizmo::SCALE) {
    ImGui::SameLine();
    ImGui::Text("  Mode:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL)) {
      mCurrentGizmoMode = ImGuizmo::LOCAL;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD)) {
      mCurrentGizmoMode = ImGuizmo::WORLD;
    }
  }
  ImGui::End();

  if (!isPlayMode_) {
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(vMin.x, vMin.y, vMax.x - vMin.x, vMax.y - vMin.y);
  } else {
    return; // PlayMode中はGizmoの操作や描画を行わない
  }

  ICamera* camera = GetActiveCamera();
  if (!camera) return;
  Matrix4x4 viewMatrix = camera->GetViewMatrix();
  Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();

  // ======================================
  // 敵の Gizmo 編集
  // ======================================
  if (currentSelectType_ == EditorSelectType::Enemy && selectedEnemyIndex_ >= 0 && selectedEnemyIndex_ < editorEnemies_.size()) {
    Enemy* selected = editorEnemies_[selectedEnemyIndex_].get();
    if (!selected->IsDead()) {
      Transform& t = selected->GetTransform();
      Matrix4x4 worldMatrix = MakeAffineMatrix(t.scale, t.rotate, t.translate);

      if (ImGuizmo::Manipulate(&viewMatrix.m[0][0], &projectionMatrix.m[0][0], mCurrentGizmoOperation, mCurrentGizmoMode, &worldMatrix.m[0][0])) {
        float matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents(&worldMatrix.m[0][0], matrixTranslation, matrixRotation, matrixScale);
        t.translate = {matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]};
        
        // マイナススケール対策: スケールを常に正にする
        t.scale = {abs(matrixScale[0]), abs(matrixScale[1]), abs(matrixScale[2])};
        
        // 角度はそのまま適用
        t.rotate = {
            matrixRotation[0] * (std::numbers::pi_v<float> / 180.0f),
            matrixRotation[1] * (std::numbers::pi_v<float> / 180.0f),
            matrixRotation[2] * (std::numbers::pi_v<float> / 180.0f)};
      }
    }
  }
  // ======================================
  // SceneObject の Gizmo 編集
  // ======================================
  else if (currentSelectType_ == EditorSelectType::SceneObject && selectedSceneObjectIndex_ >= 0 && selectedSceneObjectIndex_ < sceneObjects_.size()) {
    Object3d* selected = sceneObjects_[selectedSceneObjectIndex_].get();
    Vector3 t = selected->GetTranslation();
    Vector3 r = selected->GetRotation();
    Vector3 s = selected->GetScale();
    
    Matrix4x4 worldMatrix = MakeAffineMatrix(s, r, t);

    if (ImGuizmo::Manipulate(&viewMatrix.m[0][0], &projectionMatrix.m[0][0], mCurrentGizmoOperation, mCurrentGizmoMode, &worldMatrix.m[0][0])) {
      float matrixTranslation[3], matrixRotation[3], matrixScale[3];
      ImGuizmo::DecomposeMatrixToComponents(&worldMatrix.m[0][0], matrixTranslation, matrixRotation, matrixScale);
      t = {matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]};
      
      // マイナススケール対策: スケールを常に正にする
      s = {abs(matrixScale[0]), abs(matrixScale[1]), abs(matrixScale[2])};
      
      // 角度はそのまま適用
      r = {
          matrixRotation[0] * (std::numbers::pi_v<float> / 180.0f),
          matrixRotation[1] * (std::numbers::pi_v<float> / 180.0f),
          matrixRotation[2] * (std::numbers::pi_v<float> / 180.0f)};
          
      selected->SetTranslation(t);
      selected->SetRotation(r);
      selected->SetScale(s);
    }
  }
  // ======================================
  // Waypoint の Gizmo 編集
  // ======================================
  else if (isWaypointMode) {
    if (railCamera_) {
      auto& waypoints = railCamera_->GetWaypointsRef();
      if (selectedWaypointIndex_ < waypoints.size()) {
        Vector3 wp = waypoints[selectedWaypointIndex_];
        Matrix4x4 worldMatrix = MakeTranslateMatrix(wp);

        ImGuizmo::Manipulate(&viewMatrix.m[0][0], &projectionMatrix.m[0][0], mCurrentGizmoOperation, mCurrentGizmoMode, &worldMatrix.m[0][0]);

        if (ImGuizmo::IsUsing()) {
          waypoints[selectedWaypointIndex_] = {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
        }
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
  auto& activeEnemies = isPlayMode_ ? runtimeEnemies_ : editorEnemies_;
  for (auto& enemy : activeEnemies) {
    if (!enemy->IsDead()) {
      enemy->Draw3D();
    }
  }
  for (auto& obj : sceneObjects_) {
    obj->Draw();
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

void GamePlayScene::SaveLevel(const std::string& filename) {
  nlohmann::json root;
  nlohmann::json enemiesArray = nlohmann::json::array();
  
  for (auto& enemy : editorEnemies_) {
    if (enemy->IsDead()) continue;
    Transform& t = enemy->GetTransform();
    nlohmann::json eJson;
    eJson["translation"] = {t.translate.x, t.translate.y, t.translate.z};
    eJson["rotation"] = {t.rotate.x, t.rotate.y, t.rotate.z};
    eJson["scale"] = {t.scale.x, t.scale.y, t.scale.z};
    eJson["tag"] = enemy->GetTag();
    enemiesArray.push_back(eJson);
  }
  root["enemies"] = enemiesArray;

  nlohmann::json spawnEventsArray = nlohmann::json::array();
  for (auto& ev : spawnEvents_) {
    nlohmann::json evJson;
    evJson["spawnTime"] = ev.spawnTime;
    evJson["prefabName"] = ev.prefabName;
    evJson["spawnOffset"] = {ev.spawnOffset.x, ev.spawnOffset.y, ev.spawnOffset.z};
    spawnEventsArray.push_back(evJson);
  }
  root["spawnEvents"] = spawnEventsArray;

  nlohmann::json sceneObjectsArray = nlohmann::json::array();
  for (auto& obj : sceneObjects_) {
    nlohmann::json oJson;
    Vector3 t = obj->GetTranslation();
    Vector3 r = obj->GetRotation();
    Vector3 s = obj->GetScale();
    oJson["translation"] = {t.x, t.y, t.z};
    oJson["rotation"] = {r.x, r.y, r.z};
    oJson["scale"] = {s.x, s.y, s.z};
    oJson["modelPath"] = obj->GetModelPath();
    oJson["tag"] = obj->tag_;
    sceneObjectsArray.push_back(oJson);
  }
  root["sceneObjects"] = sceneObjectsArray;

  // Waypoint の保存
  nlohmann::json waypointsArray = nlohmann::json::array();
  if (railCamera_) {
    for (const auto& wp : railCamera_->GetWaypoints()) {
      waypointsArray.push_back({wp.x, wp.y, wp.z});
    }
  }
  root["waypoints"] = waypointsArray;

  // 保存先フォルダ（resources/levels）が存在しない場合は作成する
  std::filesystem::create_directories("resources/levels");

  // Save to the specified file path
  std::string filePath = "resources/levels/" + filename;
  std::ofstream file(filePath);
  if (file.is_open()) {
    file << std::setw(4) << root << std::endl;
  }
}

void GamePlayScene::LoadLevel(const std::string& filename) {
  std::string filePath = "resources/levels/" + filename;
  std::ifstream file(filePath);
  if (!file.is_open()) return;

  nlohmann::json root;
  file >> root;

  // 古い敵をクリア
  editorEnemies_.clear();
  runtimeEnemies_.clear();
  selectedEnemyIndex_ = -1;
  sceneObjects_.clear();
  selectedSceneObjectIndex_ = -1;
  spawnEvents_.clear();
  selectedSpawnEventIndex_ = -1;

  if (root.contains("enemies")) {
    for (auto& eJson : root["enemies"]) {
      Vector3 t = {eJson["translation"][0], eJson["translation"][1], eJson["translation"][2]};
      Vector3 r = {eJson["rotation"][0], eJson["rotation"][1], eJson["rotation"][2]};
      Vector3 s = {eJson["scale"][0], eJson["scale"][1], eJson["scale"][2]};
      
      auto dummy = std::make_unique<Enemy>();
      dummy->Initialize();
      if (eJson.contains("tag")) dummy->SetTag(eJson["tag"]);
      
      auto dummyModel = std::make_unique<Object3d>();
      dummyModel->Initialize(engine_->GetObject3dRenderer());
      dummyModel->SetModel("suzanne.obj");
      dummyModel->SetColor({1.0f, 0.2f, 0.2f, 1.0f});
      dummy->SetModel(std::move(dummyModel));
      dummy->SetBaseColor({1.0f, 0.2f, 0.2f, 1.0f});
      
      dummy->GetTransform().translate = t;
      dummy->GetTransform().rotate = r;
      dummy->GetTransform().scale = s;
      
      editorEnemies_.push_back(std::move(dummy));
    }
  }

  if (root.contains("spawnEvents")) {
    for (auto& evJson : root["spawnEvents"]) {
      SpawnEvent ev;
      ev.spawnTime = evJson["spawnTime"];
      ev.prefabName = evJson["prefabName"];
      if (evJson.contains("spawnOffset")) {
        ev.spawnOffset = {evJson["spawnOffset"][0], evJson["spawnOffset"][1], evJson["spawnOffset"][2]};
      } else if (evJson.contains("spawnPosition")) { // 古いセーブデータ互換性
        ev.spawnOffset = {0.0f, 0.0f, 50.0f};
      }
      ev.hasSpawned = false;
      spawnEvents_.push_back(ev);
    }
  }

  if (root.contains("sceneObjects")) {
    for (auto& oJson : root["sceneObjects"]) {
      std::string path = oJson["modelPath"];
      Vector3 t = {oJson["translation"][0], oJson["translation"][1], oJson["translation"][2]};
      Vector3 r = {oJson["rotation"][0], oJson["rotation"][1], oJson["rotation"][2]};
      Vector3 s = {oJson["scale"][0], oJson["scale"][1], oJson["scale"][2]};
      SpawnSceneObject(path, t);
      auto& obj = sceneObjects_.back();
      obj->SetRotation(r);
      obj->SetScale(s);
      if (oJson.contains("tag")) obj->tag_ = oJson["tag"];
    }
  }

  // Waypoint の読み込み
  if (root.contains("waypoints")) {
    std::vector<Vector3> loadedWaypoints;
    for (auto& wJson : root["waypoints"]) {
      loadedWaypoints.push_back({wJson[0], wJson[1], wJson[2]});
    }
    if (railCamera_ && !loadedWaypoints.empty()) {
      railCamera_->Initialize(loadedWaypoints);
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
  dummy->SetBaseColor({1.0f, 0.2f, 0.2f, 1.0f});

  dummy->GetTransform().translate = position;
  dummy->GetTransform().scale = {3.0f, 3.0f, 3.0f};

  editorEnemies_.push_back(std::move(dummy));
}

void GamePlayScene::SpawnSceneObject(const std::string& modelPath, const Vector3& position) {
  auto obj = std::make_unique<Object3d>();
  obj->Initialize(engine_->GetObject3dRenderer());
  obj->SetModel(modelPath);
  obj->SetTranslation(position);
  sceneObjects_.push_back(std::move(obj));
}

void GamePlayScene::OnFileDropped(const std::string& filePath) {
  // Game View のカメラの前方に配置するなど工夫できますが、今回は原点付近に配置
  Vector3 spawnPos = {0.0f, 0.0f, 0.0f};
  if (camera_) {
    spawnPos = camera_->GetTranslate() + Vector3{0.0f, 0.0f, 10.0f}; // カメラの少し前
  }
  SpawnSceneObject(filePath, spawnPos);
  
  // 生成後、すぐ選択状態にする
  currentSelectType_ = EditorSelectType::SceneObject;
  selectedSceneObjectIndex_ = static_cast<int>(sceneObjects_.size() - 1);
}