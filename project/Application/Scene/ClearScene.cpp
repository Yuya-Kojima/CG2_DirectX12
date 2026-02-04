#include "ClearScene.h"
#include "Core/EngineBase.h"
#include "Input/Input.h"
#include "Math/MathUtil.h"
#include "Render/Camera/GameCamera.h"
#include "Render/Model/ModelManager.h"
#include "Render/Object3d/Object3d.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Renderer/Object3dRenderer.h"
#include "Render/Sprite/Sprite.h"
#include "Scene/SceneManager.h"
#include "Audio/SoundManager.h"

void ClearScene::Initialize(EngineBase *engine) {
  engine_ = engine;

  //===========================
  // スプライト関係の初期化
  //===========================
  clearSprite_ = std::make_unique<Sprite>();
  clearSprite_->Initialize(engine_->GetSpriteRenderer(),
                           "resources/Clear/gameClear.png");
  clearSprite_->SetPosition(Vector2{640.0f, 200.0f});
  clearSprite_->SetAnchorPoint(Vector2{0.5f, 0.5f});
  clearSprite_->SetSize(Vector2{900.0f, 300.0f});

  uvTransform_ = {
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  toTitle = std::make_unique<Sprite>();
  toTitle->Initialize(engine_->GetSpriteRenderer(),
                      "resources/operation_UI/Button2_UI.png");
  toTitle->SetPosition(Vector2{900.0f, 600.0f});
  toTitle->SetSize(Vector2{400.0f, 100.0f});

  uvToTitle_ = {
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  //===========================
  // 3Dオブジェクト関係の初期化
  //===========================

  // カメラの生成と初期化
  camera_ = std::make_unique<GameCamera>();
  camera_->SetRotate({0.3f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 4.0f, -10.0f});
  engine_->GetObject3dRenderer()->SetDefaultCamera(camera_.get());

  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f},
      {0.3f, 0.0f, 0.0f},
      {0.0f, 4.0f, -10.0f},
  };

  ModelManager::GetInstance()->LoadModel("player_body.obj");
  player_ = std::make_unique<Object3d>();
  player_->Initialize(engine_->GetObject3dRenderer());
  player_->SetModel("player_body.obj");
  player_->SetRotation(Vector3{0.0f, 3.14f, 0.0f});
  playerBasePos_ = {0.0f, 0.0f, 0.0f};
  player_->SetTranslation(playerBasePos_);

  // --- 天球モデルの用意 ---
  ModelManager::GetInstance()->LoadModel("SkyDome.obj");
  skyObject3d_ = std::make_unique<Object3d>();
  skyObject3d_->Initialize(engine_->GetObject3dRenderer());
  skyObject3d_->SetModel("SkyDome.obj");
  skyObject3d_->SetTranslation({0.0f, 0.0f, 0.0f});
  skyObject3d_->SetScale({skyScale_, skyScale_, skyScale_});
  skyObject3d_->SetRotation({0.0f, 0.0f, 0.0f});
  skyObject3d_->SetEnableLighting(false);
  skyObject3d_->SetColor(Vector4{3.0f, 3.0f, 3.0f, 1.0f});

  //===========================
  // パーティクル関係の初期化
  //===========================

  ParticleManager::GetInstance()->CreateParticleGroup("Clear_Fountain",
                                                      "resources/circle.png");
  ParticleManager::GetInstance()->CreateParticleGroup("Clear_Burst",
                                                      "resources/circle.png");
  ParticleManager::GetInstance()->CreateParticleGroup("Clear_Glitter",
                                                      "resources/circle.png");

  clearFountain_ = std::make_unique<ParticleEmitter>(
      "Clear_Fountain", Vector3{0.0f, -3.0f, 2.0f}, // center
      Vector3{0.25f, 0.0f, 0.25f},                  // halfSize
      10,                                           // count
      0.20f,                                        // frequency
      Vector3{0.0f, 6.0f, 0.0f},                    // baseVel
      Vector3{2.5f, 1.0f, 2.5f}, 0.6f, 1.1f);
  clearFountain_->SetBaseScale(Vector3{0.25f, 0.25f, 0.25f});
  clearFountain_->SetScaleRandom(Vector3{0.10f, 0.10f, 0.10f});
  clearFountain_->SetColor(Vector4{1.0f, 0.9f, 0.2f, 0.8f});

  clearBurst_ = std::make_unique<ParticleEmitter>(
      "Clear_Burst",
      Vector3{0.0f, 1.0f,
              2.0f}, // center（プレイヤーの少し上にすると気持ちいい）
      Vector3{0.05f, 0.05f, 0.05f}, // halfSize（ほぼ一点）
      60,                           // count（ドン！）
      0.0f,                         // frequency（自動発生OFF）
      Vector3{0.0f, 2.5f, 0.0f},    // baseVel
      Vector3{7.0f, 5.0f, 7.0f},    // velRandom（全方向に弾ける）
      0.5f, 0.9f                    // life（短め）
  );
  clearBurst_->SetBaseScale(Vector3{0.18f, 0.18f, 0.18f});
  clearBurst_->SetScaleRandom(Vector3{0.10f, 0.10f, 0.10f});
  clearBurst_->SetRotateRandom(Vector3{0.0f, 0.0f, 6.28f}); // くるくる
  clearBurst_->SetColor(Vector4{1.0f, 1.0f, 1.0f, 0.95f});

  clearGlitter_ = std::make_unique<ParticleEmitter>(
      "Clear_Glitter", Vector3{0.0f, 2.0f, 2.0f}, // center（上から）
      Vector3{1.0f, 0.2f, 1.0f},                  // halfSize（広めに散る）
      6,                                          // count
      0.12f,                                      // frequency（細かく出す）
      Vector3{0.0f, -0.6f, 0.0f},                 // baseVel（ゆっくり落ちる）
      Vector3{0.6f, 0.2f, 0.6f},                  // velRandom
      0.9f, 1.6f                                  // life（余韻）
  );
  clearGlitter_->SetBaseScale({0.22f, 0.22f, 0.22f});
  clearGlitter_->SetScaleRandom({0.12f, 0.12f, 0.12f});

  auto* sm = SoundManager::GetInstance();
  // キー名は "title_bgm" / "title_se"
  // resources配下のファイルを指定
  sm->Load("title_bgm", "resources/sounds/BGM/clearBGM.mp3");
  sm->Load("select_se", "resources/sounds/SE/select.mp3");
  sm->Load("push_se", "resources/sounds/SE/push.mp3");
  // タイトル開始時にBGMをループ再生
  sm->PlayBGM("title_bgm");
}

void ClearScene::Finalize() {

    auto* sm = SoundManager::GetInstance();
    sm->StopBGM();
    sm->StopAllSE();
    // 登録したキーをアンロード
    sm->Unload("title_bgm");
    sm->Unload("push_se");
    sm->Unload("select_se");

  // 出ているパーティクルをすべてクリア
  ParticleManager::GetInstance()->ClearAllParticles();
}

void ClearScene::Update() {

  const float dt = 1.0f / 60.0f;

  // input取得
  auto *input = engine_->GetInputManager();

  if (engine_->GetInputManager()->IsKeyTrigger(KeyCode::Space) ||
      engine_->GetInputManager()->IsPadTrigger(PadButton::A)) {
      SoundManager::GetInstance()->PlaySE("push_se");
    SceneManager::GetInstance()->ChangeScene("STAGESELECT");
  } else if (engine_->GetInputManager()->IsKeyTrigger(KeyCode::T) ||
             engine_->GetInputManager()->IsPadTrigger(PadButton::Start)) {
      SoundManager::GetInstance()->PlaySE("push_se");
    SceneManager::GetInstance()->ChangeScene("TITLE");
  }

  clearSprite_->Update(uvTransform_);
  toTitle->Update(uvToTitle_);

  //=======================
  // 3Dオブジェクトの更新
  //=======================

  if (player_) {
    playerJumpTime_ += dt;

    float tInCycle = std::fmod(playerJumpTime_, jumpCycleSec_);

    float yOffset = 0.0f;

    if (tInCycle < groundHoldSec_) {
      yOffset = 0.0f;
    } else {
      float jumpPhaseLen = jumpCycleSec_ - groundHoldSec_;
      float u = (tInCycle - groundHoldSec_) / jumpPhaseLen;

      float h = 0.0f;
      if (u < 0.5f) {
        float t = u / 0.5f;
        h = EaseOutCubic(t);
      } else {
        float t = (u - 0.5f) / 0.5f;
        h = 1.0f - EaseInCubic(t);
      }

      yOffset = h * jumpHeight_;
    }

    Vector3 p = playerBasePos_;
    p.y += yOffset;

    player_->SetTranslation(p);
    player_->Update();

    Vector3 playerPos = p;
    if (clearFountain_)
      clearFountain_->SetCenter(
          {playerPos.x, playerPos.y - 3.0f, playerPos.z + 2.0f});
    if (clearGlitter_)
      clearGlitter_->SetCenter(
          {playerPos.x, playerPos.y + 2.0f, playerPos.z + 2.0f});
    if (clearBurst_)
      clearBurst_->SetCenter(
          {playerPos.x, playerPos.y + 1.0f, playerPos.z + 2.0f});
  }

  // 天球の更新
  if (skyObject3d_) {
    const ICamera *activeCam =
        engine_->GetObject3dRenderer()->GetDefaultCamera();
    if (activeCam) {
      Vector3 camPos = activeCam->GetTranslate();
      skyObject3d_->SetTranslation({camPos.x, camPos.y, camPos.z});
    }
    skyRotate_ += 0.0005f;
    if (skyRotate_ > 3.14159265f * 2.0f)
      skyRotate_ -= 3.14159265f * 2.0f;
    skyObject3d_->SetRotation({0.0f, skyRotate_, 0.0f});
    skyObject3d_->Update();
  }

  //=======================
  // パーティクルの更新
  //=======================

  /* エミッタの更新(パーティクルを生成間隔に基づいて自動生成)
  --------------------------------------------------*/
  if (clearFountain_)
    clearFountain_->Update();
  if (clearGlitter_)
    clearGlitter_->Update();

  if (!didPlayClearBurst_) {
    didPlayClearBurst_ = true;
    if (clearBurst_) {
      clearBurst_->Emit(Vector4{1.0f, 1.0f, 1.0f, 0.95f});
      clearBurst_->Emit(Vector4{1.0f, 0.9f, 0.2f, 0.85f});
    }
  }

  //=======================
  // カメラの更新
  //=======================
  const ICamera *activeCamera = nullptr;

  camera_->SetRotate(cameraTransform_.rotate);
  camera_->SetTranslate(cameraTransform_.translate);

  // カメラの更新処理
  camera_->Update();
  activeCamera = camera_.get();

  // アクティブカメラを描画で使用する
  engine_->GetObject3dRenderer()->SetDefaultCamera(activeCamera);

  // view / projection を作って ParticleManager 更新
  ParticleManager::GetInstance()->Update(activeCamera->GetViewMatrix(),
                                         activeCamera->GetProjectionMatrix());
}

void ClearScene::Draw() {
  Draw3D();
  Draw2D();
}

void ClearScene::Draw3D() {
  engine_->Begin3D();

  // ここから下で3DオブジェクトのDrawを呼ぶ
  if (skyObject3d_) {
    skyObject3d_->Draw();
  }

  if (player_) {
    player_->Draw();
  }

  ParticleManager::GetInstance()->Draw("Clear_Fountain");
  ParticleManager::GetInstance()->Draw("Clear_Burst");
  ParticleManager::GetInstance()->Draw("Clear_Glitter");
}

void ClearScene::Draw2D() {
  engine_->Begin2D();

  // ここから下で2DオブジェクトのDrawを呼ぶ
  clearSprite_->Draw();
  toTitle->Draw();
}

ClearScene::~ClearScene() = default;
