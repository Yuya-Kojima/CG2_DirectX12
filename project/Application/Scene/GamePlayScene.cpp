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
#include <string>
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
    player_->SetCamera(activeCamera);
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
  ImGui::Begin("Hierarchy & Inspector");

  if (ImGui::Button("Save Level")) {
    SaveLevel();
  }
  ImGui::SameLine();
  if (ImGui::Button("Load Level")) {
    LoadLevel();
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Enemy")) {
    Vector3 camPos = activeCamera->GetTranslate();
    AddEnemy({camPos.x, camPos.y + 5.0f, camPos.z + 50.0f});
  }

  ImGui::Spacing();
  ImGui::Text("Enemy Hierarchy");
  ImGui::Separator();

  // 敵のリストを表示
  for (size_t i = 0; i < enemies_.size(); ++i) {
    if (enemies_[i]->IsDead()) continue;

    std::string label = "Enemy " + std::to_string(i);
    if (ImGui::Selectable(label.c_str(), selectedEnemyIndex_ == static_cast<int>(i))) {
      selectedEnemyIndex_ = static_cast<int>(i);
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Inspector");

  if (selectedEnemyIndex_ >= 0 && selectedEnemyIndex_ < enemies_.size()) {
    Enemy* selected = enemies_[selectedEnemyIndex_].get();
    if (!selected->IsDead()) {
      Transform& t = selected->GetTransform();
      ImGui::DragFloat3("Translate", &t.translate.x, 0.1f);
      ImGui::DragFloat3("Rotate", &t.rotate.x, 0.01f);
      ImGui::DragFloat3("Scale", &t.scale.x, 0.1f);
    } else {
      selectedEnemyIndex_ = -1; // 死んだら選択解除
    }
  } else {
    ImGui::Text("No enemy selected.");
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