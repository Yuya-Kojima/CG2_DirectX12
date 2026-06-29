#include "GamePlayScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Debug/ImGuiManager.h"
#include "Debug/Logger.h"
#include "Framework/UIManager.h"
#include "Input/InputKeyState.h"
#include "Model/Model.h"
#include "Audio/SoundManager.h"
#include <fstream>
#include <filesystem>
#include "../../externals/nlohmann/json.hpp"
#include "../Effect/EffectManager.h"
#include "Object3d/Object3d.h"
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Render/Particle/BillboardParticleEmitter.h"
#include "Render/Particle/MeshParticleEmitter.h"
#include "Model/ModelManager.h"
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
      : scene_(scene), index_(idx), oldEvent_(oldEv), newEvent_(newEv) {}
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
#ifdef USE_IMGUI
#include "ImGuizmo.h"
#include <imgui.h>
#endif
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
  EffectManager::GetInstance()->Initialize();

  //===========================
  // スプライト関係の初期化
  //===========================

  //===========================
  // 3Dオブジェクト関係の初期化
  //===========================
  for (int i = 0; i < kMaxHitEffects; ++i) {
    hitCoreParticleGroups_[i] = std::make_unique<BillboardParticleEmitter>();
    hitCoreParticleGroups_[i]->Initialize("resources/circle.png");
    hitFlareParticleGroups_[i] = std::make_unique<BillboardParticleEmitter>();
    hitFlareParticleGroups_[i]->Initialize("resources/circle.png");
    hitRingParticleGroups_[i] = std::make_unique<BillboardParticleEmitter>();
    hitRingParticleGroups_[i]->Initialize("resources/circle.png");
    hitRingParticleGroups_[i]->SetIsRingMode(true);

    // 1. コア
    deathCoreEmitters_[i] = std::make_unique<ParticleEmitter>(
        hitCoreParticleGroups_[i].get(), Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 1, 0.0f,
        Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 0.5f, 0.5f);
    deathCoreEmitters_[i]->SetBaseScale({20.0f, 20.0f, 20.0f});
    deathCoreEmitters_[i]->SetColor({1.0f, 0.8f, 0.8f, 1.0f});
    deathCoreEmitters_[i]->SetScaleVelocity({-20.0f, -20.0f, -20.0f});

    // 2. フレア
    deathFlareEmitters_[i] = std::make_unique<ParticleEmitter>(
        hitFlareParticleGroups_[i].get(), Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.5f, 0.5f, 0.5f}, 40, 0.0f,
        Vector3{-30.0f, -30.0f, -30.0f}, Vector3{30.0f, 30.0f, 30.0f}, 0.4f, 0.6f);
    deathFlareEmitters_[i]->SetBaseScale({0.8f, 0.8f, 0.8f});
    deathFlareEmitters_[i]->SetColor({2.0f, 0.6f, 0.1f, 1.0f});
    deathFlareEmitters_[i]->SetScaleVelocity({-1.0f, -1.0f, -1.0f});

    // 3. リング衝撃波
    deathRingEmitters_[i] = std::make_unique<ParticleEmitter>(
        hitRingParticleGroups_[i].get(), Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 1, 0.0f,
        Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 0.7f, 0.7f);
    deathRingEmitters_[i]->SetBaseScale({0.1f, 0.1f, 0.1f});
    deathRingEmitters_[i]->SetColor({2.0f, 0.2f, 0.1f, 1.0f});
    deathRingEmitters_[i]->SetScaleVelocity({80.0f, 80.0f, 80.0f});
  }

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

    // ゲーム状態のリセット（Stop時に綺麗な状態に戻す）
    gameState_ = GameState::Play;
    UIManager::GetInstance()->Load("resources/UI/GamePlayUI.json");

    // 残っている敵や弾をクリア
    runtimeEnemies_.clear();
    ActorManager::GetInstance()->Clear();

    // プレイヤーのステータスを初期化
    if (player_) {
      player_->SetLoadConfigOnInitialize(false);
      player_->Initialize();
    }

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

  // ポストエフェクトとエフェクトマネージャーの更新
  if (postProcess_) {
    Matrix4x4 viewProj = Multiply(railCamera_->GetViewMatrix(), railCamera_->GetProjectionMatrix());
    EffectManager::GetInstance()->Update(viewProj);

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

          // 撃破時の全体エフェクト演出コールバックを登録
          Enemy* enemyPtr = newEnemy.get();
          newEnemy->SetOnDestroyedCallback([this, enemyPtr](bool isSelfDestruct) {
            // 自爆の場合は通常の撃破エフェクトを出さずに終了する
            if (isSelfDestruct) {
                // 必要であればカメラ揺れだけ起こすなど
                if (railCamera_) railCamera_->Shake(0.5f, 0.2f);
                return;
            }

            EffectManager::GetInstance()->PlayShockwave(enemyPtr->GetTransform().translate);
            if (railCamera_) {
              railCamera_->Shake(1.0f, 0.3f); // カメラを強く揺らす
            }
            
            // パーティクル発生
            int i = nextHitEffectIndex_;
            deathCoreEmitters_[i]->SetCenter(enemyPtr->GetTransform().translate);
            deathCoreEmitters_[i]->Emit();
            deathFlareEmitters_[i]->SetCenter(enemyPtr->GetTransform().translate);
            deathFlareEmitters_[i]->Emit();
            deathRingEmitters_[i]->SetCenter(enemyPtr->GetTransform().translate);
            deathRingEmitters_[i]->Emit();
            
            nextHitEffectIndex_ = (nextHitEffectIndex_ + 1) % kMaxHitEffects;
          });

          // カメラとオフセット、プレイヤー情報をセット
          newEnemy->SetCamera(railCamera_.get());
          newEnemy->SetPlayer(player_.get());
          newEnemy->SetSpawnOffset(ev.spawnOffset);
          // MoveTypeはプレハブから読み込まれるためここでは上書きしない

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

  if (ImGui::Selectable("Player", currentSelectType_ == EditorSelectType::Player)) {
    currentSelectType_ = EditorSelectType::Player;
  }
  if (ImGui::Selectable("Environment", currentSelectType_ == EditorSelectType::Environment)) {
    currentSelectType_ = EditorSelectType::Environment;
  }
  if (ImGui::Selectable("Effect (Shockwave)", currentSelectType_ == EditorSelectType::Effect)) {
    currentSelectType_ = EditorSelectType::Effect;
  }
  ImGui::Separator();
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
  if (ImGui::TreeNodeEx("Prefabs (Master Data)", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (std::filesystem::exists("resources/prefabs")) {
      for (const auto& entry : std::filesystem::directory_iterator("resources/prefabs")) {
        if (entry.path().extension() == ".prefab") {
          std::string name = entry.path().stem().string();
          bool isSelected = (currentSelectType_ == EditorSelectType::Prefab && selectedPrefabName_ == name);
          if (ImGui::Selectable(name.c_str(), isSelected)) {
            currentSelectType_ = EditorSelectType::Prefab;
            if (selectedPrefabName_ != name) {
              selectedPrefabName_ = name;
              tempPrefabEditEnemy_ = PrefabManager::GetInstance()->InstantiateEnemy(name, Transform());
            }
          }
        }
      }
    } else {
      ImGui::TextDisabled("No prefabs found.");
    }
    ImGui::TreePop();
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

  if (currentSelectType_ == EditorSelectType::Player) {
    ImGui::Text("Player Action Settings");
    ImGui::Separator();
    if (player_) {
      bool isDirty = player_->IsActionConfigDirty();
      
      if (isDirty) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.0f, 1.0f)); // 警告色（オレンジ）
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.7f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
      }
      
      std::string buttonText = isDirty ? (const char*)u8"[* 未保存] Save Config" : (const char*)u8"Save Config";
      if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
        player_->SaveActionConfig();
        isDirty = false;
      }
      
      if (isDirty) {
        ImGui::PopStyleColor(3);
      }
      
      ImGui::Spacing();
      
      auto& config = player_->GetActionConfig();
      bool changed = false;
      
      changed |= ImGui::SliderFloat((const char*)u8"ロックオンの吸いつき範囲", &config.lockOnRadius, 10.0f, 500.0f);
      changed |= ImGui::SliderFloat((const char*)u8"ホーミング弾のスピード", &config.homingSpeed, 0.1f, 5.0f);
      changed |= ImGui::SliderInt((const char*)u8"追尾を開始するまでのフレーム", &config.homingFallTime, 0, 300);
      changed |= ImGui::SliderFloat((const char*)u8"追尾のカーブの鋭さ", &config.homingStrengthIncrease, 0.001f, 0.1f);
      changed |= ImGui::SliderFloat((const char*)u8"旋回力（追尾力）", &config.homingStrengthMax, 0.01f, 1.0f);
      changed |= ImGui::SliderFloat((const char*)u8"照準の加速度", &config.reticleAcceleration, 0.1f, 10.0f);
      changed |= ImGui::SliderFloat((const char*)u8"照準の摩擦力", &config.reticleFriction, 0.5f, 0.99f);
      changed |= ImGui::SliderFloat((const char*)u8"照準の最高速度", &config.reticleMaxSpeed, 1.0f, 100.0f);
      changed |= ImGui::SliderFloat((const char*)u8"ロール（バンク）の強さ", &config.rollStrength, 0.0f, 20.0f);
      changed |= ImGui::SliderFloat((const char*)u8"ピッチ（上下）の追従強度", &config.pitchStrength, 0.0f, 10.0f);
      changed |= ImGui::SliderFloat((const char*)u8"ヨー（首振り）の追従強度", &config.yawStrength, 0.0f, 10.0f);
      changed |= ImGui::SliderFloat((const char*)u8"ロールの補間速度（Lerp）", &config.rollLerp, 0.01f, 1.0f);
      changed |= ImGui::SliderFloat((const char*)u8"ピッチの補間速度（Lerp）", &config.pitchLerp, 0.01f, 1.0f);
      changed |= ImGui::SliderFloat((const char*)u8"ヨーの補間速度（Lerp）", &config.yawLerp, 0.01f, 1.0f);
      
      if (changed) {
        player_->SetActionConfigDirty(true);
      }
    } else {
      ImGui::Text("Player is not active.");
    }
  } else if (currentSelectType_ == EditorSelectType::Environment) {
    ImGui::Text("Environment Settings");
    ImGui::Separator();
    auto pp = SceneManager::GetInstance()->GetCurrentScenePostProcess();
    if (pp) {
      pp->DrawDebugUI("Environment", false); // インライン描画
    } else {
      ImGui::Text("No PostProcess active.");
    }
  } else if (currentSelectType_ == EditorSelectType::Effect) {
    EffectManager::GetInstance()->DrawEditorUI(railCamera_.get());
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

    // プレハブのコンボボックス
    std::vector<std::string> availablePrefabs;
    if (std::filesystem::exists("resources/prefabs")) {
      for (const auto& entry : std::filesystem::directory_iterator("resources/prefabs")) {
        if (entry.path().extension() == ".prefab") {
          availablePrefabs.push_back(entry.path().stem().string());
        }
      }
    }
    
    if (ImGui::BeginCombo("Prefab", ev.prefabName.c_str())) {
      for (const auto& pName : availablePrefabs) {
        bool isSelected = (ev.prefabName == pName);
        if (ImGui::Selectable(pName.c_str(), isSelected)) {
          if (ev.prefabName != pName) {
            ev.prefabName = pName;
            editFinished = true;
          }
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    if (ImGui::DragFloat3("Spawn Offset", &ev.spawnOffset.x, 0.1f)) {
      editFinished = true;
    }
    if (railCamera_) {
      Vector3 camPos = railCamera_->CalcPosition(ev.spawnTime);
      Vector3 nextPos = railCamera_->CalcPosition(
          std::min(ev.spawnTime + 0.01f,
                   static_cast<float>(railCamera_->GetWaypointsRef().size() - 1)));
      Vector3 forwardDir = {nextPos.x - camPos.x, nextPos.y - camPos.y,
                            nextPos.z - camPos.z};
      forwardDir = SafeNormalize(forwardDir);

      Vector3 camTrans = camPos;
      camTrans.y += 2.5f;

      Vector3 camRot = {0.0f, 0.0f, 0.0f};
      camRot.y = std::atan2(forwardDir.x, forwardDir.z);
      float xzLen = std::sqrt(forwardDir.x * forwardDir.x +
                              forwardDir.z * forwardDir.z);
      camRot.x = std::atan2(-forwardDir.y, xzLen) + 0.1f;

      Matrix4x4 camWorld = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, camRot, camTrans);
      Vector3 right = {camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2]};
      Vector3 up = {camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2]};
      Vector3 forward = {camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2]};
      Vector3 camActualPos = {camWorld.m[3][0], camWorld.m[3][1],
                              camWorld.m[3][2]};

      Vector3 worldPos = camActualPos + 
                         Vector3{right.x * ev.spawnOffset.x, right.y * ev.spawnOffset.x, right.z * ev.spawnOffset.x} +
                         Vector3{up.x * ev.spawnOffset.y, up.y * ev.spawnOffset.y, up.z * ev.spawnOffset.y} +
                         Vector3{forward.x * ev.spawnOffset.z, forward.y * ev.spawnOffset.z, forward.z * ev.spawnOffset.z};

      ImGui::Spacing();
      ImGui::TextDisabled("World Pos: (%.1f, %.1f, %.1f)", worldPos.x, worldPos.y, worldPos.z);
    }
    
    // MoveType はプレハブ側の設定に移動したため、ここには表示しない

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
  } else if (currentSelectType_ == EditorSelectType::Prefab && tempPrefabEditEnemy_) {
    ImGui::Text("Prefab Master Settings: %s", selectedPrefabName_.c_str());
    ImGui::Separator();
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
    ImGui::PopStyleColor();
    ImGui::Spacing();

    bool changed = false;
    
    int hp = tempPrefabEditEnemy_->GetHP();
    if (ImGui::InputInt("HP (体力)", &hp)) {
      if (hp < 1) hp = 1;
      tempPrefabEditEnemy_->SetHP(hp);
      changed = true;
    }
    
    float speed = tempPrefabEditEnemy_->GetSpeed();
    if (ImGui::DragFloat("Speed (移動速度)", &speed, 0.01f, 0.0f, 10.0f)) {
      tempPrefabEditEnemy_->SetSpeed(speed);
      changed = true;
    }

    const char* moveTypes[] = {
      "Straight (直進)", 
      "Parallel (平行移動)", 
      "SineWave (波打ち)", 
      "Stationary (静止)", 
      "Fighter (戦闘機)", 
      "Meteor (メテオ突撃)", 
      "Strafe (画面横断)", 
      "Turret (固定砲台)"
    };
    int currentMoveType = static_cast<int>(tempPrefabEditEnemy_->GetMoveType());
    if (ImGui::Combo("Move Type (行動パターン)", &currentMoveType, moveTypes, IM_ARRAYSIZE(moveTypes))) {
      tempPrefabEditEnemy_->SetMoveType(static_cast<MoveType>(currentMoveType));
      changed = true;
    }

    Vector3 dir = tempPrefabEditEnemy_->GetMoveDirection();
    if (ImGui::DragFloat3("Move Direction (移動方向)", &dir.x, 0.01f)) {
      tempPrefabEditEnemy_->SetMoveDirection(dir);
      changed = true;
    }

    ImGui::Spacing();
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f));
    if (ImGui::Button((const char*)u8"Save Prefab (上書き保存)", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
      PrefabManager::GetInstance()->SavePrefab(selectedPrefabName_, tempPrefabEditEnemy_.get());
    }
    ImGui::PopStyleColor();
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
    ev.spawnTime = railCamera_ ? railCamera_->GetT() : 0.0f;
    ev.prefabName = "ZakoEnemy";
    ev.spawnOffset = {0.0f, 0.0f, 50.0f};

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

  // デバッグ用の線を描画
  const ICamera *activeCamera = GetActiveCamera();
  if (activeCamera == nullptr) {
    activeCamera = railCamera_.get();
  }
#ifdef USE_IMGUI
  if (useDebugCamera_) {
    activeCamera = debugCamera_->GetCamera();
  }

  // デバッグ描画
  CollisionManager::GetInstance()->DrawDebug();

  engine_->GetLineRenderer()->Render(activeCamera->GetViewProjectionMatrix());
#endif

  // パーティクルの更新と発生（ルートシグネチャが変わるため、3Dモデル描画後に）
  ParticleManager::GetInstance()->Update();
  if (activeCamera) {
    for (int i = 0; i < kMaxHitEffects; ++i) {
      hitCoreParticleGroups_[i]->Update(activeCamera->GetViewMatrix(), activeCamera->GetProjectionMatrix());
      hitFlareParticleGroups_[i]->Update(activeCamera->GetViewMatrix(), activeCamera->GetProjectionMatrix());
      hitRingParticleGroups_[i]->Update(activeCamera->GetViewMatrix(), activeCamera->GetProjectionMatrix());
    }
  }
  ParticleManager::GetInstance()->Emit();

  for (int i = 0; i < kMaxHitEffects; ++i) {
    hitCoreParticleGroups_[i]->Draw();
    hitFlareParticleGroups_[i]->Draw();
    hitRingParticleGroups_[i]->Draw();
  }

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
      } else if (evJson.contains("spawnPosition")) { // 古いセーブデータ互換
        ev.spawnOffset = {0.0f, 0.0f, 50.0f};
      }
      // moveType は読まない（Prefab側で管理）
      
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
