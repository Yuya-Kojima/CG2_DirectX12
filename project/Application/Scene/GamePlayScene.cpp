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

  sprite_ = std::make_unique<Sprite>();
  sprite_->Initialize(engine_->GetSpriteRenderer(), "resources/white1x1.png");

  spritePosition_ = {
      100.0f,
      100.0f,
  };

  sprite_->SetPosition(spritePosition_);

  uvTransformSprite_ = {
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

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
  // スプライトの更新
  //=======================

#ifdef USE_IMGUI
  //========================
  // Sprite座標をImGuiで操作
  //========================
  ImGui::SetNextWindowSize(ImVec2(500.0f, 100.0f), ImGuiCond_Once);

  ImGui::Begin("Sprite Pos");

  ImGui::SliderFloat2("position", &spritePosition_.x, 0.0f, 1280.0f, "%.1f");

  ImGui::End();
#endif

  sprite_->SetPosition(spritePosition_);
  sprite_->SetSize({100.0f, 100.0f});
  sprite_->Update(uvTransformSprite_);

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
  // レールカメラの進行度（t_）が0.1（少し進んだところ）を超えたら1回だけスポーン
  if (!hasSpawnedDummy_ && railCamera_->GetT() > 0.1f) {
    hasSpawnedDummy_ = true;

    auto dummy = std::make_unique<Enemy>();
    dummy->Initialize(); // Colliderの登録など

    auto dummyModel = std::make_unique<Object3d>();
    dummyModel->Initialize(engine_->GetObject3dRenderer());
    dummyModel->SetModel("suzanne.obj"); // 仮のモデル
    dummyModel->SetColor({1.0f, 0.2f, 0.2f, 1.0f}); // 赤色
    dummy->SetModel(std::move(dummyModel));

    // カメラの少し奥（前進方向）に出現させる
    Vector3 camPos = railCamera_->GetTranslate();
    
    dummy->GetTransform().translate = {camPos.x, camPos.y + 10.0f, camPos.z + 150.0f};
    dummy->GetTransform().scale = {3.0f, 3.0f, 3.0f}; // 少し大きめ

    enemies_.push_back(std::move(dummy));
  } // ここでダミー生成のif文を閉じる

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

  engine_->End3D();
}

void GamePlayScene::Draw2D() {
  // ここから下で2DオブジェクトのDrawを呼ぶ
  sprite_->Draw();

  if (player_) {
    player_->Draw2D();
  }
}