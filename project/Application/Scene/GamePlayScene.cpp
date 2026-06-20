#include "GamePlayScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Debug/ImGuiManager.h"
#include "Debug/Logger.h"
#include "Framework/UIManager.h"
#include "Input/InputKeyState.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Render/Particle/BillboardParticleEmitter.h"
#include "Render/Particle/MeshParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/SceneManager.h"
#include "Sprite/Sprite.h"
#include "Texture/TextureManager.h"
#include "Math/MathUtil.h"

#include "../Editor/CommandManager.h"
#include "Collision/CollisionManager.h"
#include "Framework/ActorManager.h"
#include "Framework/GameManager.h"
#include "Framework/PrefabManager.h"
#include "Renderer/PostProcess.h"

// ======================================
// Undo/Redo用コマンドクラス
// ======================================
class CmdAddSpawnEvent : public ICommand {
  GamePlayScene *scene_;
  SpawnEvent event_;
  int index_;

public:
  CmdAddSpawnEvent(GamePlayScene *scene, const SpawnEvent &ev, int idx)
      : scene_(scene), event_(ev), index_(idx) {}
  void Execute() override {
    auto &events = scene_->GetSpawnEvents();
    events.insert(events.begin() + index_, event_);
    scene_->SelectSpawnEvent(index_);
  }
  void Undo() override {
    auto &events = scene_->GetSpawnEvents();
    events.erase(events.begin() + index_);
    scene_->SelectSpawnEvent(-1);
  }
};

class CmdDeleteSpawnEvent : public ICommand {
  GamePlayScene *scene_;
  SpawnEvent event_;
  int index_;

public:
  CmdDeleteSpawnEvent(GamePlayScene *scene, int idx)
      : scene_(scene), index_(idx) {
    event_ = scene_->GetSpawnEvents()[idx];
  }
  void Execute() override {
    auto &events = scene_->GetSpawnEvents();
    events.erase(events.begin() + index_);
    scene_->SelectSpawnEvent(-1);
  }
  void Undo() override {
    auto &events = scene_->GetSpawnEvents();
    events.insert(events.begin() + index_, event_);
    scene_->SelectSpawnEvent(index_);
  }
};

class CmdModifySpawnEvent : public ICommand {
  GamePlayScene *scene_;
  int index_;
  SpawnEvent oldEvent_;
  SpawnEvent newEvent_;

public:
  CmdModifySpawnEvent(GamePlayScene *scene, int idx, const SpawnEvent &oldEv,
                      const SpawnEvent &newEv)
      : scene_(scene), index_(idx), oldEvent_(oldEv), newEvent_(newEvent_) {}
  void Execute() override {
    scene_->GetSpawnEvents()[index_] = newEvent_;
    scene_->SelectSpawnEvent(index_);
  }
  void Undo() override {
    scene_->GetSpawnEvents()[index_] = oldEvent_;
    scene_->SelectSpawnEvent(index_);
  }
};

// ======================================
#include "../../externals/nlohmann/json.hpp"
#include "ImGuizmo.h"
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iomanip>
#include <numbers>
#include <string>

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
  hitCoreParticleGroup_ = std::make_unique<BillboardParticleEmitter>();
  hitCoreParticleGroup_->Initialize("resources/circle.png");
  hitFlareParticleGroup_ = std::make_unique<BillboardParticleEmitter>();
  hitFlareParticleGroup_->Initialize("resources/circle.png");
  hitRingParticleGroup_ = std::make_unique<BillboardParticleEmitter>();
  hitRingParticleGroup_->Initialize("resources/circle.png");
  hitRingParticleGroup_->SetIsRingMode(true);

  // デバッグカメラの初期化（開発用の自由カメラ）真っ直ぐ奥へ進むだけの自然なレールに変更）
  waypoints_ = {{0.0f, 4.0f, -10.0f}, {0.0f, 4.0f, 40.0f},
                {0.0f, 4.0f, 90.0f},  {0.0f, 4.0f, 140.0f},
                {0.0f, 4.0f, 190.0f}, {0.0f, 4.0f, 240.0f}};
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
  skybox_->SetColor({0.6f,0.6f,0.6f,1.0f});

  //===========================
  // プレイヤーの初期化
  //===========================
  ModelManager::GetInstance()->LoadModel("suzanne.obj");

  player_ = std::make_unique<Player>(railCamera_.get());
  player_->SetSpriteRenderer(engine_->GetSpriteRenderer());
  player_->SetObject3dRenderer(engine_->GetObject3dRenderer());
  player_->SetInput(engine_->GetInputManager());

  // プレイヤーの初期化（照準やコライダーの生成など）
  player_->Initialize();
  auto playerModel = std::make_unique<Object3d>();
  playerModel->Initialize(engine_->GetObject3dRenderer());
  playerModel->SetModel("suzanne.obj"); // 仮の自機モデル
  playerModel->SetColor({0.0f, 0.5f, 1.0f, 1.0f});
  player_->SetModel(std::move(playerModel));

  // 環境マッピングのテスト用オブジェクト（メタリックなモンスターボール）
  ModelManager::GetInstance()->LoadModel("monsterBall.obj");
  metallicObject_ = std::make_unique<Object3d>();
  metallicObject_->Initialize(engine_->GetObject3dRenderer());
  metallicObject_->SetModel("monsterBall.obj");
  metallicObject_->SetEnvironmentCoefficient(1.0f);     // 100%反射
  metallicObject_->SetTranslation({0.0f, 5.0f, 50.0f}); // レール上の奥に配置
  metallicObject_->SetScale({3.0f, 3.0f, 3.0f});        // 少し大きめに
  metallicObject_->Update();

  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f},
      {0.3f, 0.0f, 0.0f},
      {0.0f, 4.0f, -10.0f},
  };

  // デバッグカメラ
  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize({0.0f, 10.0f, -30.0f});

  // レベルデータのロード
  LoadLevel();
  // 古いハードコード生成が走らないようにtrueにしておく
  hasSpawnedDummy_ = true;

  // UIの読み込み
  UIManager::GetInstance()->Load("resources/UI/GamePlayUI.json");
}

void GamePlayScene::Finalize() {}

void GamePlayScene::Update() {

  bool isPlayMode_ = GameManager::GetInstance()->IsGlobalPlayMode();

  if (isPlayMode_ && !previousGlobalPlayMode_) {
    SaveLevel("level_editor_temp.json");
    isPaused_ = false;
    useDebugCamera_ = false;
    if (railCamera_) {
      railCamera_->SetAutoMove(true);
      playStartT_ = railCamera_->GetT();
    }
    for (auto &ev : spawnEvents_) {
      ev.hasSpawned = false;
    }
  } else if (!isPlayMode_ && previousGlobalPlayMode_) {
    isPaused_ = false;
    useDebugCamera_ = true;
    LoadLevel("level_editor_temp.json");
    if (railCamera_) {
      railCamera_->SetT(playStartT_);
      bool autoMoveCache = railCamera_->GetAutoMove();
      railCamera_->SetAutoMove(false);
      railCamera_->Update();
      railCamera_->SetAutoMove(autoMoveCache);
      for (auto &ev : spawnEvents_) {
        ev.hasSpawned = false;
      }
    }
    if (player_) {
      player_->ForceSnapToCamera();
    }
  }
  previousGlobalPlayMode_ = isPlayMode_;

#ifdef USE_IMGUI
  // Undo/Redo ショートカット (Ctrl + Z / Ctrl + Y)
  // Playモード中は編集操作のUndo/Redoを禁止する
  if (!isPlayMode_ && (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) ||
                       ImGui::IsKeyDown(ImGuiKey_RightCtrl))) {
    if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
      CommandManager::GetInstance()->Undo();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
      CommandManager::GetInstance()->Redo();
    }
  }
#endif

  // Sound更新
  SoundManager::GetInstance()->Update();

  // UI更新
  Input *input = GameManager::GetInstance()->IsGlobalPlayMode()
                     ? engine_->GetInputManager()
                     : nullptr;
  UIManager::GetInstance()->Update(input);

  // デバッグカメラ切り替え
  if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
    if (useDebugCamera_) {
      useDebugCamera_ = false;
    } else {
      useDebugCamera_ = true;
    }
  }

  //=======================
  // ゲーム進行状態の更新
  //=======================
  if (gameState_ == GameState::Play) {
    if (railCamera_ && railCamera_->IsFinished()) {
      gameState_ = GameState::Clear;
      if (railCamera_)
        railCamera_->SetAutoMove(false);
      UIManager::GetInstance()->Load("resources/UI/ClearUI.json");
    } else if (player_ && player_->IsDead()) {
      gameState_ = GameState::GameOver;
      if (railCamera_)
        railCamera_->SetAutoMove(false);
      UIManager::GetInstance()->Load("resources/UI/GameOverUI.json");
    }
  } else if (gameState_ == GameState::Clear ||
             gameState_ == GameState::GameOver) {
    // リザルト画面でEnterキーを押したらステージセレクト（またはタイトル）へ戻る
    if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
      SceneManager::GetInstance()->SetNextTransitionFade(0.5f);
      SceneManager::GetInstance()->ChangeScene(
          gameState_ == GameState::Clear ? "STAGE_SELECT" : "TITLE");
    }
  }

  bool shouldUpdateLogic = (isPlayMode_ && !isPaused_) || doStep_;

  if (doStep_) {
    doStep_ = false;
  }

  // ポストエフェクトの更新
  if (postProcess_) {
    if (hitEffectTimer_ > 0.0f) {
      if (shouldUpdateLogic) {
        hitEffectTimer_ -= 1.0f / 60.0f;
      }
      float t = hitEffectTimer_ / 0.5f; // 1.0 -> 0.0に減衰
      
      // しきい値(Threshold)を極端に上げる(例:10.0f)ことで、SkyboxのHDR的な白さも無視し、
      // パーティクルが加算合成で重なりまくった「爆発のコア」だけを強烈に光らせる
      postProcess_->SetBloomIntensity(1.0f + t * 2.5f); // 4.0から2.5に少しマイルド化
      postProcess_->SetBloomThreshold(10.0f); 
      postProcess_->SetUseBloom(true);

      // ショックウェーブ（空間の歪み）の適用
      postProcess_->SetPostEffectType(10); // 10: Shockwave
      
      // 敵のワールド座標をスクリーン(UV)座標に変換する
      Matrix4x4 viewProj = Multiply(railCamera_->GetViewMatrix(), railCamera_->GetProjectionMatrix());
      Vector3 pos = lastDestroyedEnemyPos_;
      float w = pos.x * viewProj.m[0][3] + pos.y * viewProj.m[1][3] + pos.z * viewProj.m[2][3] + viewProj.m[3][3];
      Vector3 ndcPos = {
          (pos.x * viewProj.m[0][0] + pos.y * viewProj.m[1][0] + pos.z * viewProj.m[2][0] + viewProj.m[3][0]) / w,
          (pos.x * viewProj.m[0][1] + pos.y * viewProj.m[1][1] + pos.z * viewProj.m[2][1] + viewProj.m[3][1]) / w,
          (pos.x * viewProj.m[0][2] + pos.y * viewProj.m[1][2] + pos.z * viewProj.m[2][2] + viewProj.m[3][2]) / w
      };
      
      // NDCをUVに変換
      float uvX = (ndcPos.x + 1.0f) * 0.5f;
      float uvY = (1.0f - ndcPos.y) * 0.5f;
      
      postProcess_->SetShockwaveCenter(uvX, uvY);
      // 広がり：0.0 から 0.8 へ拡大 (t は 1.0->0.0)
      postProcess_->SetShockwaveRadius((1.0f - t) * 0.8f);
      // 波紋の太さ：0.05 から 0.2 へ太くなる
      float thickness = 0.05f * t + 0.2f * (1.0f - t);
      postProcess_->SetShockwaveThickness(thickness);
      // 歪みの強さ：最初は強く、徐々に消える
      postProcess_->SetShockwaveWeight(t);
      postProcess_->SetShockwaveDistortion(0.05f);
    } else {
      postProcess_->SetUseBloom(false);
      postProcess_->SetBloomIntensity(1.0f);
      postProcess_->SetBloomThreshold(0.8f); // デフォルト値に戻す
      
      // ポストエフェクトを通常に戻す
      postProcess_->SetPostEffectType(0);
      postProcess_->SetShockwaveWeight(0.0f);
    }
  }
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
      for (auto &ev : spawnEvents_) {
        if (t < ev.spawnTime) {
          ev.hasSpawned = false; // シークバック時にフラグをリセット
        } else if (shouldUpdateLogic && !ev.hasSpawned && t >= ev.spawnTime) {
          // スポーン (カメラの現在位置からの相対座標で計算)
          Matrix4x4 viewMatrix = railCamera_->GetViewMatrix();
          Matrix4x4 cameraWorld = Inverse(viewMatrix);
          Vector3 cameraPos = {cameraWorld.m[3][0], cameraWorld.m[3][1],
                               cameraWorld.m[3][2]};
          Vector3 cameraRight = {cameraWorld.m[0][0], cameraWorld.m[0][1],
                                 cameraWorld.m[0][2]};
          Vector3 cameraUp = {cameraWorld.m[1][0], cameraWorld.m[1][1],
                              cameraWorld.m[1][2]};
          Vector3 cameraForward = {cameraWorld.m[2][0], cameraWorld.m[2][1],
                                   cameraWorld.m[2][2]};

          Vector3 spawnWorldPos = cameraPos +
                                  Vector3{cameraRight.x * ev.spawnOffset.x,
                                          cameraRight.y * ev.spawnOffset.x,
                                          cameraRight.z * ev.spawnOffset.x} +
                                  Vector3{cameraUp.x * ev.spawnOffset.y,
                                          cameraUp.y * ev.spawnOffset.y,
                                          cameraUp.z * ev.spawnOffset.y} +
                                  Vector3{cameraForward.x * ev.spawnOffset.z,
                                          cameraForward.y * ev.spawnOffset.z,
                                          cameraForward.z * ev.spawnOffset.z};

          // 敵の生成
          auto newEnemy = PrefabManager::GetInstance()->InstantiateEnemy(
              ev.prefabName,
              Transform{{3.0f, 3.0f, 3.0f}, {0, 0, 0}, spawnWorldPos});

          // パーティクルエミッターを渡す
          newEnemy->SetParticleEmitters(hitCoreParticleGroup_.get(), hitFlareParticleGroup_.get(), hitRingParticleGroup_.get());

          // 旧式の進行方向（フォールバック用）
          newEnemy->SetMoveDirection(
              {-cameraForward.x, -cameraForward.y, -cameraForward.z});

          // 撃破時の全体エフェクト演出コールバックを登録
          Enemy* enemyPtr = newEnemy.get();
          newEnemy->SetOnDestroyedCallback([this, enemyPtr]() {
            hitEffectTimer_ = 0.5f; // 0.5秒のフラッシュ・歪み演出
            if (railCamera_) {
              railCamera_->Shake(1.0f, 0.3f); // カメラを強く揺らす
            }
            lastDestroyedEnemyPos_ = enemyPtr->GetTransform().translate;
          });

          // カメラとオフセット、MoveTypeをセット
          newEnemy->SetCamera(railCamera_.get());
          newEnemy->SetSpawnOffset(ev.spawnOffset);
          newEnemy->SetMoveType(static_cast<MoveType>(ev.moveType));

          // Playモード時のみ実際の敵を生成
          if (isPlayMode_) {
            runtimeEnemies_.push_back(std::move(newEnemy));
          }
          ev.hasSpawned = true;
        }
      }
    }
  }

  // 基底クラスにも現在のアクティブカメラを教える（Gizmo描画などで使うため）
  SetActiveCamera(const_cast<ICamera *>(activeCamera));

  // 敵の更新 (Playモードで生成された敵のみ)
  for (auto &enemy : runtimeEnemies_) {
    if (shouldUpdateLogic) {
      enemy->Update();
    } else {
      enemy->UpdateTransform();
    }
  }
  for (auto &obj : sceneObjects_) {
    obj->Update();
  }

  // LockOn用にPlayerに敵リストを渡す（毎回最新の状態を渡す）
  enemyPtrs_.clear();
  for (auto &e : runtimeEnemies_) {
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

  if (metallicObject_) {
    // ゆっくり回転させて環境マップの反射を分かりやすくする
    Vector3 rot = metallicObject_->GetRotation();
    rot.y += 0.01f;
    metallicObject_->SetRotation(rot);
    metallicObject_->Update();
  }

  // プレイヤーの更新
  if (player_) {
    // プレイヤーの照準や挙動の計算には常にレールカメラを使用する
    // GameState::Play
    // 以外の時（クリア後など）は操作を受け付けないようにUpdateTransformのみ呼ぶ
    if (shouldUpdateLogic && gameState_ == GameState::Play) {
      player_->Update();
    } else {
      player_->UpdateTransform();
    }

    // ロックオン中は画面をグレースケールにする
    if (postProcess_) {
      if (player_->IsLockOnMode()) {
        postProcess_->SetUseGrayscale(true);
      } else {
        postProcess_->SetUseGrayscale(false);
      }
    }
  }

  // アクター群の更新
  ActorManager::GetInstance()->Update();

  // 当たり判定の更新
  if (shouldUpdateLogic) {
    CollisionManager::GetInstance()->Update();
  }

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

    ImGui::EndMainMenuBar();
  }

  //=========================
  // Hierarchy ウィンドウ
  //=========================
  ImGui::Begin("Hierarchy");
  ImGui::Text("Scene Objects");
  ImGui::Separator();

  if (ImGui::Selectable("Environment (PostProcess)",
                        currentSelectType_ == EditorSelectType::Environment)) {
    currentSelectType_ = EditorSelectType::Environment;
  }
  bool isRailCameraOpen = ImGui::TreeNodeEx(
      "Rail Camera", ImGuiTreeNodeFlags_OpenOnArrow |
                         ImGuiTreeNodeFlags_OpenOnDoubleClick |
                         (currentSelectType_ == EditorSelectType::RailCamera &&
                                  selectedWaypointIndex_ == -1
                              ? ImGuiTreeNodeFlags_Selected
                              : 0));
  if (ImGui::IsItemClicked()) {
    currentSelectType_ = EditorSelectType::RailCamera;
    selectedWaypointIndex_ = -1;
  }
  if (isRailCameraOpen) {
    if (railCamera_) {
      auto &waypoints = railCamera_->GetWaypointsRef();
      for (size_t i = 0; i < waypoints.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        std::string wpLabel = "Waypoint " + std::to_string(i);
        bool wpSelected = (currentSelectType_ == EditorSelectType::RailCamera &&
                           selectedWaypointIndex_ == static_cast<int>(i));
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
  ImGui::Text("Spawn Events (Timeline)");
  ImGui::Separator();

  // タイムラインピンのリストを表示
  for (size_t i = 0; i < spawnEvents_.size(); ++i) {
    char label[128];
    snprintf(label, sizeof(label), "[%zu] %s (t=%.2f)", i,
             spawnEvents_[i].prefabName.c_str(), spawnEvents_[i].spawnTime);

    bool isSelected = (currentSelectType_ == EditorSelectType::SpawnEvent &&
                       selectedSpawnEventIndex_ == static_cast<int>(i));
    if (ImGui::Selectable(label, isSelected)) {
      SelectSpawnEvent(static_cast<int>(i));
    }
  }

  ImGui::Spacing();
  ImGui::Text("Scene Objects (Dynamic)");
  ImGui::Separator();
  for (size_t i = 0; i < sceneObjects_.size(); ++i) {
    std::string label = "Object " + std::to_string(i) + " (" +
                        sceneObjects_[i]->GetModelPath() + ")";
    bool isSelected = (currentSelectType_ == EditorSelectType::SceneObject &&
                       selectedSceneObjectIndex_ == static_cast<int>(i));
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
      auto &waypoints = railCamera_->GetWaypointsRef();
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
  } else if (currentSelectType_ == EditorSelectType::SceneObject &&
             selectedSceneObjectIndex_ >= 0 &&
             selectedSceneObjectIndex_ < sceneObjects_.size()) {
    Object3d *selected = sceneObjects_[selectedSceneObjectIndex_].get();
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
    if (ImGui::DragFloat3("Translate", &t.x, 0.1f))
      selected->SetTranslation(t);
    if (ImGui::DragFloat3("Rotate", &r.x, 0.01f))
      selected->SetRotation(r);
    if (ImGui::DragFloat3("Scale", &s.x, 0.1f))
      selected->SetScale(s);
  } else if (currentSelectType_ == EditorSelectType::SpawnEvent &&
             selectedSpawnEventIndex_ >= 0 &&
             selectedSpawnEventIndex_ < spawnEvents_.size()) {
    SpawnEvent &ev = spawnEvents_[selectedSpawnEventIndex_];

    // 編集前状態の保存用
    static SpawnEvent s_editStartState;
    static int s_lastSelectedIndex = -1;
    bool isAnyEditActive = ImGui::IsAnyItemActive() || ImGuizmo::IsUsing();

    if (!isAnyEditActive || s_lastSelectedIndex != selectedSpawnEventIndex_) {
      s_editStartState = ev;
    }
    s_lastSelectedIndex = selectedSpawnEventIndex_;
    bool editFinished = false;

    ImGui::Text("Spawn Event %d", selectedSpawnEventIndex_);
    ImGui::Separator();

    // 最大時間（レール終点）を取得
    float maxT = 0.0f;
    if (railCamera_ && railCamera_->GetWaypoints().size() > 0) {
      maxT = static_cast<float>(railCamera_->GetWaypoints().size() - 1);
    }

    ImGui::DragFloat("Time", &ev.spawnTime, 0.01f, 0.0f, maxT);
    if (ImGui::IsItemDeactivatedAfterEdit())
      editFinished = true;

    char prefabBuf[64] = {0};
    snprintf(prefabBuf, sizeof(prefabBuf), "%s", ev.prefabName.c_str());
    if (ImGui::InputText("Prefab", prefabBuf, sizeof(prefabBuf))) {
      ev.prefabName = prefabBuf;
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
      editFinished = true;

    ImGui::DragFloat3("Offset", &ev.spawnOffset.x, 0.1f);
    if (ImGui::IsItemDeactivatedAfterEdit())
      editFinished = true;

    const char *moveTypeNames[] = {
        "Straight (奥から手前へ直進)", "Parallel (カメラと並走して追従)",
        "SineWave (波打って接近)", "Stationary (固定砲台・静止)"};
    if (ImGui::Combo("Move Type", &ev.moveType, moveTypeNames,
                     IM_ARRAYSIZE(moveTypeNames))) {
      editFinished = true;
    }

    if (editFinished) {
      CommandManager::GetInstance()->ExecuteCommand(
          std::make_unique<CmdModifySpawnEvent>(this, selectedSpawnEventIndex_,
                                                s_editStartState, ev));
      s_editStartState = ev;
    }

    ImGui::Spacing();
    if (ImGui::Button("Delete Event", ImVec2(-1, 0))) {
      CommandManager::GetInstance()->ExecuteCommand(
          std::make_unique<CmdDeleteSpawnEvent>(this,
                                                selectedSpawnEventIndex_));
    }
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
  if (maxT < 0.0f)
    maxT = 0.0f;
  // Playモード中はスライダーの操作を無効化
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
      player_->ForceSnapToCamera();
    }
  }

  if (isPlayMode_) {
    ImGui::EndDisabled();
  }

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImVec2 p = ImGui::GetCursorScreenPos();
  float trackWidth = ImGui::GetContentRegionAvail().x;
  float trackHeight = 40.0f;

  // 背景描画
  drawList->AddRectFilled(p, ImVec2(p.x + trackWidth, p.y + trackHeight),
                          IM_COL32(50, 50, 50, 255));

  // マウス操作判定用の非表示ボタン
  ImGui::InvisibleButton("TimelineTrack", ImVec2(trackWidth, trackHeight));
  bool isTrackHovered = ImGui::IsItemHovered();

  // ダブルクリックで新規スポーンイベント追加
  if (isTrackHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    float mouseX = ImGui::GetMousePos().x - p.x;
    float t = (mouseX / trackWidth) * maxT;
    SpawnEvent ev;
    ev.spawnTime = t;
    ev.prefabName = "ZakoEnemy";
    ev.spawnOffset = {0.0f, 0.0f, 50.0f};
    ev.moveType = 0; // MoveType::Straight

    int newIndex = static_cast<int>(spawnEvents_.size());
    CommandManager::GetInstance()->ExecuteCommand(
        std::make_unique<CmdAddSpawnEvent>(this, ev, newIndex));
  }

  // イベントのピンを描画
  for (size_t i = 0; i < spawnEvents_.size(); ++i) {
    float evX = p.x + (spawnEvents_[i].spawnTime / maxT) * trackWidth;
    ImVec2 evCenter(evX, p.y + trackHeight / 2.0f);

    // 選択状態なら色を変える
    bool isSelected = (currentSelectType_ == EditorSelectType::SpawnEvent &&
                       selectedSpawnEventIndex_ == static_cast<int>(i));
    ImU32 col =
        isSelected ? IM_COL32(255, 200, 0, 255) : IM_COL32(100, 150, 250, 255);

    // ひし形描画
    float size = 8.0f;
    drawList->AddQuadFilled(ImVec2(evCenter.x, evCenter.y - size),
                            ImVec2(evCenter.x + size, evCenter.y),
                            ImVec2(evCenter.x, evCenter.y + size),
                            ImVec2(evCenter.x - size, evCenter.y), col);

    // クリック判定
    if (isTrackHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      float mouseX = ImGui::GetMousePos().x;
      if (abs(mouseX - evX) < size * 1.5f) {
        currentSelectType_ = EditorSelectType::SpawnEvent;
        selectedSpawnEventIndex_ = static_cast<int>(i);
      }
    }
  }

  // 現在の再生位置(Playhead)を描画
  float playheadX = p.x + (currentT / maxT) * trackWidth;
  drawList->AddLine(ImVec2(playheadX, p.y),
                    ImVec2(playheadX, p.y + trackHeight),
                    IM_COL32(255, 0, 0, 255), 2.0f);

  ImGui::End();

  // レールのデバッグ描画（スプライン曲線をサンプリングして描画）
  const auto &waypoints = railCamera_->GetWaypoints();
  if (waypoints.size() > 1) {
    float maxPathT = static_cast<float>(waypoints.size() - 1);
    const float step = 0.05f; // サンプリング間隔
    Vector3 prevPos = railCamera_->CalcPosition(0.0f);

    // スプライン曲線を滑らかに描画する
    for (float t = step; t <= maxPathT; t += step) {
      Vector3 currentPos = railCamera_->CalcPosition(t);
      engine_->GetLineRenderer()->DrawLine(
          prevPos, currentPos, {0.0f, 1.0f, 1.0f, 1.0f}); // シアン色の線
      prevPos = currentPos;
    }
    // 端数調整（最後の終点まで確実に結ぶ）
    engine_->GetLineRenderer()->DrawLine(
        prevPos, railCamera_->CalcPosition(maxPathT), {0.0f, 1.0f, 1.0f, 1.0f});

    // 現在のカメラ位置に赤い目印をつける（短い線でクロスを描く等）
    Vector3 camPos = railCamera_->CalcPosition(currentT);
    engine_->GetLineRenderer()->DrawLine({camPos.x - 2, camPos.y, camPos.z},
                                         {camPos.x + 2, camPos.y, camPos.z},
                                         {1.0f, 0.0f, 0.0f, 1.0f});
    engine_->GetLineRenderer()->DrawLine({camPos.x, camPos.y - 2, camPos.z},
                                         {camPos.x, camPos.y + 2, camPos.z},
                                         {1.0f, 0.0f, 0.0f, 1.0f});
    engine_->GetLineRenderer()->DrawLine({camPos.x, camPos.y, camPos.z - 2},
                                         {camPos.x, camPos.y, camPos.z + 2},
                                         {1.0f, 0.0f, 0.0f, 1.0f});
  }
#endif
}
void GamePlayScene::DrawEditorUI() {
#ifdef USE_IMGUI
  // ======================================
  // 共通的 Gizmo UI および状態管理
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

  // 共通の描画領域取得 (ImGui::Image
  // の直後に呼ばれる前提なので、直前のItemRectがGame Viewの画像サイズになる)
  ImVec2 vMin = ImGui::GetItemRectMin();
  ImVec2 vMax = ImGui::GetItemRectMax();

  // Gizmo用UIを Main Toolbar に追記
  ImGui::Begin("Main Toolbar");

  bool isPlayMode_ = GameManager::GetInstance()->IsGlobalPlayMode();
  if (isPlayMode_) {
    if (isPaused_) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
      if (ImGui::Button(" [ RESUME ] ", ImVec2(80, 0))) {
        isPaused_ = false;
      }
      ImGui::PopStyleColor();
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.2f, 1.0f));
      if (ImGui::Button(" [ STEP ] ", ImVec2(80, 0))) {
        doStep_ = true;
      }
      ImGui::PopStyleColor();
    } else {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.2f, 1.0f));
      if (ImGui::Button(" [ PAUSE ] ", ImVec2(80, 0))) {
        isPaused_ = true;
      }
      ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    ImGui::Text(" | ");
  }
  ImGui::SameLine();
  ImGui::Text("Gizmo:");
  ImGui::SameLine();

  // Waypoint選択時はTranslateモードに固定する
  bool isWaypointMode = (currentSelectType_ == EditorSelectType::RailCamera &&
                         selectedWaypointIndex_ >= 0);
  if (isWaypointMode) {
    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::TextDisabled("Rotate [E]");
    ImGui::SameLine();
    ImGui::TextDisabled("Scale [R]");
  } else {
    if (ImGui::RadioButton("Translate [W]",
                           mCurrentGizmoOperation == ImGuizmo::TRANSLATE)) {
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate [E]",
                           mCurrentGizmoOperation == ImGuizmo::ROTATE)) {
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale [R]",
                           mCurrentGizmoOperation == ImGuizmo::SCALE)) {
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

  ICamera *camera = GetActiveCamera();
  if (!camera)
    return;
  Matrix4x4 viewMatrix = camera->GetViewMatrix();
  Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();

  // ======================================
  // SceneObject の Gizmo 編集
  // ======================================
  if (currentSelectType_ == EditorSelectType::SceneObject &&
      selectedSceneObjectIndex_ >= 0 &&
      selectedSceneObjectIndex_ < sceneObjects_.size()) {
    Object3d *selected = sceneObjects_[selectedSceneObjectIndex_].get();
    Vector3 t = selected->GetTranslation();
    Vector3 r = selected->GetRotation();
    Vector3 s = selected->GetScale();

    Matrix4x4 worldMatrix = MakeAffineMatrix(s, r, t);

    if (ImGuizmo::Manipulate(&viewMatrix.m[0][0], &projectionMatrix.m[0][0],
                             mCurrentGizmoOperation, mCurrentGizmoMode,
                             &worldMatrix.m[0][0])) {
      float matrixTranslation[3], matrixRotation[3], matrixScale[3];
      ImGuizmo::DecomposeMatrixToComponents(
          &worldMatrix.m[0][0], matrixTranslation, matrixRotation, matrixScale);
      t = {matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]};

      // マイナススケール対策: スケールを常に正にする
      s = {abs(matrixScale[0]), abs(matrixScale[1]), abs(matrixScale[2])};

      // 角度はそのまま適用
      r = {matrixRotation[0] * (std::numbers::pi_v<float> / 180.0f),
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
      auto &waypoints = railCamera_->GetWaypointsRef();
      if (selectedWaypointIndex_ < waypoints.size()) {
        Vector3 wp = waypoints[selectedWaypointIndex_];
        Matrix4x4 worldMatrix = MakeTranslateMatrix(wp);

        ImGuizmo::Manipulate(&viewMatrix.m[0][0], &projectionMatrix.m[0][0],
                             mCurrentGizmoOperation, mCurrentGizmoMode,
                             &worldMatrix.m[0][0]);

        if (ImGuizmo::IsUsing()) {
          waypoints[selectedWaypointIndex_] = {
              worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
        }
      }
    }
  }
  // ======================================
  // SpawnEvent の Gizmo 編集 (ゴースト)
  // ======================================
  else if (currentSelectType_ == EditorSelectType::SpawnEvent &&
           selectedSpawnEventIndex_ >= 0 &&
           selectedSpawnEventIndex_ < spawnEvents_.size()) {
    SpawnEvent &ev = spawnEvents_[selectedSpawnEventIndex_];

    if (railCamera_) {
      // 指定時間(ev.spawnTime)におけるカメラのワールド行列をシミュレーション
      Vector3 camPos = railCamera_->CalcPosition(ev.spawnTime);
      Vector3 camTarget = railCamera_->CalcPosition(ev.spawnTime + 0.1f);

      // 簡易的に前・上・右ベクトルを計算 (RailCamera内部のUpdateに類似)
      Vector3 forward = Normalize(camTarget - camPos);
      Vector3 up = {0.0f, 1.0f, 0.0f};
      Vector3 right = Normalize(Cross(up, forward));
      up = Normalize(Cross(forward, right));

      // そのカメラ位置から、ev.spawnOffset
      // 分だけローカル空間で移動させた位置がゴーストのワールド座標
      Vector3 ghostWorldPos =
          camPos +
          Vector3{right.x * ev.spawnOffset.x, right.y * ev.spawnOffset.x,
                  right.z * ev.spawnOffset.x} +
          Vector3{up.x * ev.spawnOffset.y, up.y * ev.spawnOffset.y,
                  up.z * ev.spawnOffset.y} +
          Vector3{forward.x * ev.spawnOffset.z, forward.y * ev.spawnOffset.z,
                  forward.z * ev.spawnOffset.z};

      // ゴーストのワールド行列
      Matrix4x4 ghostWorldMatrix = MakeTranslateMatrix(ghostWorldPos);

      // Gizmo で動かす (Translate のみ)
      static SpawnEvent s_gizmoStartState;
      static bool s_wasUsingGizmo = false;
      bool isUsingGizmo = ImGuizmo::IsUsing();

      if (isUsingGizmo && !s_wasUsingGizmo) {
        s_gizmoStartState = ev;
      }

      if (ImGuizmo::Manipulate(&viewMatrix.m[0][0], &projectionMatrix.m[0][0],
                               ImGuizmo::TRANSLATE, mCurrentGizmoMode,
                               &ghostWorldMatrix.m[0][0])) {
        // 動かした後の新しいワールド座標
        Vector3 newGhostPos = {ghostWorldMatrix.m[3][0],
                               ghostWorldMatrix.m[3][1],
                               ghostWorldMatrix.m[3][2]};

        // 新しいワールド座標から、カメラのローカル座標(offset)を逆算する
        Vector3 diff = newGhostPos - camPos;
        ev.spawnOffset.x =
            diff.x * right.x + diff.y * right.y + diff.z * right.z;
        ev.spawnOffset.y = diff.x * up.x + diff.y * up.y + diff.z * up.z;
        ev.spawnOffset.z =
            diff.x * forward.x + diff.y * forward.y + diff.z * forward.z;
      }

      if (!isUsingGizmo && s_wasUsingGizmo) {
        CommandManager::GetInstance()->ExecuteCommand(
            std::make_unique<CmdModifySpawnEvent>(
                this, selectedSpawnEventIndex_, s_gizmoStartState, ev));
      }
      s_wasUsingGizmo = isUsingGizmo;

      // 3D空間へのゴーストの描画
      engine_->GetLineRenderer()->DrawLine(ghostWorldPos - Vector3{1, 0, 0},
                                           ghostWorldPos + Vector3{1, 0, 0},
                                           {1, 0, 1, 1});
      engine_->GetLineRenderer()->DrawLine(ghostWorldPos - Vector3{0, 1, 0},
                                           ghostWorldPos + Vector3{0, 1, 0},
                                           {1, 0, 1, 1});
      engine_->GetLineRenderer()->DrawLine(ghostWorldPos - Vector3{0, 0, 1},
                                           ghostWorldPos + Vector3{0, 0, 1},
                                           {1, 0, 1, 1});
    }
  }
#endif
}

void GamePlayScene::Draw() { Draw3D(); }

void GamePlayScene::Draw3D() {
  engine_->Begin3D();

  if (skybox_) {
    engine_->GetSkyboxRenderer()->Begin();
    skybox_->Draw();

    // Skybox描画後に通常3Dへ戻す
    engine_->GetObject3dRenderer()->Begin();
  }

  // 敵の描画
  for (auto &enemy : runtimeEnemies_) {
    if (!enemy->IsDead()) {
      enemy->Draw3D();
    }
  }
  for (auto &obj : sceneObjects_) {
    obj->Draw();
  }

  // 環境マッピングオブジェクトの描画
  if (metallicObject_) {
    metallicObject_->Draw();
  }

  if (player_) {
    player_->Draw3D();
  }

  // アクター群（弾など）の描画
  ActorManager::GetInstance()->Draw3D();

#ifdef USE_IMGUI
  // デバッグ用の線を描画
  const ICamera *activeCamera = nullptr;
  if (useDebugCamera_) {
    activeCamera = debugCamera_->GetCamera();
  } else {
    activeCamera = GetActiveCamera();
    if (activeCamera == nullptr) {
      activeCamera = railCamera_.get();
    }
  }

  // デバッグ描画
  CollisionManager::GetInstance()->DrawDebug();

  engine_->GetLineRenderer()->Render(activeCamera->GetViewProjectionMatrix());
#endif

  // パーティクルの更新と発生（ルートシグネチャが変わるため、3Dモデル描画後に）
  ParticleManager::GetInstance()->Update();
  if (activeCamera) {
    hitCoreParticleGroup_->Update(activeCamera->GetViewMatrix(), activeCamera->GetProjectionMatrix());
    hitFlareParticleGroup_->Update(activeCamera->GetViewMatrix(), activeCamera->GetProjectionMatrix());
    hitRingParticleGroup_->Update(activeCamera->GetViewMatrix(), activeCamera->GetProjectionMatrix());
  }
  ParticleManager::GetInstance()->Emit();

  hitCoreParticleGroup_->Draw();
  hitFlareParticleGroup_->Draw();
  hitRingParticleGroup_->Draw();

  engine_->End3D();
}

void GamePlayScene::Draw2D() {
  if (player_) {
    // リザルト画面中はレティクルを消す
    if (gameState_ == GameState::Play) {
      player_->Draw2D();
    }
  }

  // スプライト（UI）の描画
  UIManager::GetInstance()->Draw();

#ifdef USE_IMGUI
  if (gameState_ == GameState::Clear) {
    ImGui::SetNextWindowPos(ImVec2(1280.0f / 2.0f, 720.0f / 2.0f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Result UI", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetWindowFontScale(4.0f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "STAGE CLEAR!");
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("Press ENTER to return to Stage Select");
    ImGui::End();

    // EnterでStageSelectSceneへ帰還
    if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
      SceneManager::GetInstance()->SetNextTransitionFade(0.5f);
      SceneManager::GetInstance()->ChangeScene("STAGE_SELECT");
    }
  } else if (gameState_ == GameState::GameOver) {
    ImGui::SetNextWindowPos(ImVec2(1280.0f / 2.0f, 720.0f / 2.0f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Game Over UI", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetWindowFontScale(4.0f);
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "GAME OVER");
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                       "Press ENTER to return to Stage Select");
    ImGui::End();

    // EnterでStageSelectSceneへ帰還
    if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
      SceneManager::GetInstance()->SetNextTransitionFade(0.5f);
      SceneManager::GetInstance()->ChangeScene("STAGE_SELECT");
    }
  }
#endif
}

void GamePlayScene::SaveLevel(const std::string &filename) {
  nlohmann::json root;
  // (静的Enemyは廃止されたため、保存しない)

  nlohmann::json spawnEventsArray = nlohmann::json::array();
  for (auto &ev : spawnEvents_) {
    nlohmann::json evJson;
    evJson["spawnTime"] = ev.spawnTime;
    evJson["prefabName"] = ev.prefabName;
    evJson["spawnOffset"] = {ev.spawnOffset.x, ev.spawnOffset.y,
                             ev.spawnOffset.z};
    evJson["moveType"] = ev.moveType;
    spawnEventsArray.push_back(evJson);
  }
  root["spawnEvents"] = spawnEventsArray;

  nlohmann::json sceneObjectsArray = nlohmann::json::array();
  for (auto &obj : sceneObjects_) {
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
    for (const auto &wp : railCamera_->GetWaypoints()) {
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

void GamePlayScene::LoadLevel(const std::string &filename) {
  std::string filePath = "resources/levels/" + filename;
  std::ifstream file(filePath);
  if (!file.is_open())
    return;

  nlohmann::json root;

  // レベルロード時は配列のインデックスや参照が完全に破壊されるため、Undo履歴をリセットする
  CommandManager::GetInstance()->Clear();

  try {
    file >> root;
  } catch (const nlohmann::json::parse_error &e) {
    Logger::Log(std::string("[GamePlayScene] LoadLevel JSON parse error: ") +
                e.what());
    return;
  }

  // 古い敵をクリア
  runtimeEnemies_.clear();
  sceneObjects_.clear();
  selectedSceneObjectIndex_ = -1;
  spawnEvents_.clear();
  selectedSpawnEventIndex_ = -1;

  // 古いセーブデータの互換性維持（enemiesキーがあっても無視する）

  if (root.contains("spawnEvents")) {
    for (auto &evJson : root["spawnEvents"]) {
      SpawnEvent ev;
      ev.spawnTime = evJson["spawnTime"];
      ev.prefabName = evJson["prefabName"];
      if (evJson.contains("spawnOffset")) {
        ev.spawnOffset = {evJson["spawnOffset"][0], evJson["spawnOffset"][1],
                          evJson["spawnOffset"][2]};
      } else if (evJson.contains("spawnPosition")) { // 古いセーブデータ互換性
        ev.spawnOffset = {0.0f, 0.0f, 50.0f};
      }
      if (evJson.contains("moveType")) {
        ev.moveType = evJson["moveType"];
      } else {
        ev.moveType = 0; // デフォはStraight
      }
      ev.hasSpawned = false;
      spawnEvents_.push_back(ev);
    }
  }

  if (root.contains("sceneObjects")) {
    for (auto &oJson : root["sceneObjects"]) {
      std::string path = oJson["modelPath"];
      Vector3 t = {oJson["translation"][0], oJson["translation"][1],
                   oJson["translation"][2]};
      Vector3 r = {oJson["rotation"][0], oJson["rotation"][1],
                   oJson["rotation"][2]};
      Vector3 s = {oJson["scale"][0], oJson["scale"][1], oJson["scale"][2]};
      SpawnSceneObject(path, t);
      auto &obj = sceneObjects_.back();
      obj->SetRotation(r);
      obj->SetScale(s);
      if (oJson.contains("tag"))
        obj->tag_ = oJson["tag"];
    }
  }

  // Waypoint の読み込み
  if (root.contains("waypoints")) {
    std::vector<Vector3> loadedWaypoints;
    for (auto &wJson : root["waypoints"]) {
      loadedWaypoints.push_back({wJson[0], wJson[1], wJson[2]});
    }
    if (railCamera_ && !loadedWaypoints.empty()) {
      railCamera_->Initialize(loadedWaypoints);
    }
  }
}

void GamePlayScene::SpawnSceneObject(const std::string &modelPath,
                                     const Vector3 &position) {
  auto obj = std::make_unique<Object3d>();
  obj->Initialize(engine_->GetObject3dRenderer());
  obj->SetModel(modelPath);
  obj->SetTranslation(position);
  sceneObjects_.push_back(std::move(obj));
}

void GamePlayScene::OnFileDropped(const std::string &filePath) {
  // Game View のカメラの前方に配置するなど工夫できますが、今回は原点付近に配置
  Vector3 spawnPos = {0.0f, 0.0f, 0.0f};
  if (camera_) {
    spawnPos =
        camera_->GetTranslate() + Vector3{0.0f, 0.0f, 10.0f}; // カメラの少し前
  }
  SpawnSceneObject(filePath, spawnPos);

  // 生成後、すぐ選択状態にする
  currentSelectType_ = EditorSelectType::SceneObject;
  selectedSceneObjectIndex_ = static_cast<int>(sceneObjects_.size() - 1);
}
