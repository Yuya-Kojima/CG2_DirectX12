#include "TitleScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Framework/GameManager.h"
#include "Framework/UIManager.h"
#include "Input/InputKeyState.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/PostProcess.h"
#include "Render/SkyBox/SkyBox.h"
#include "Scene/SceneManager.h"
#include "Sprite/Sprite.h"
#include "Texture/TextureManager.h"
#include <cmath>

// ImGuiを使用するためのインクルード
#ifdef USE_IMGUI
#include "Debug/ImGuiManager.h"
#endif

void TitleScene::Initialize(EngineBase *engine) {

  // 基底クラスの初期化（PostProcessの生成など）
  BaseScene::Initialize(engine);

  // 参照をコピー
  engine_ = engine;

  //===========================
  // テクスチャファイルの読み込み
  //===========================

  //===========================
  // オーディオファイルの読み込み
  //===========================
  SoundManager::GetInstance()->Load("ui_decide",
                                    "resources/Sounds/ui_decide.wav");
  SoundManager::GetInstance()->Load("title_bgm",
                                    "resources/Sounds/title_bgm.mp3");
  SoundManager::GetInstance()->PlayBGM("title_bgm");

  //===========================
  // スプライト関係の初期化
  //===========================

  // ディレクショナルライト設定
  if (auto *dl = engine_->GetObject3dRenderer()->GetDirectionalLightData()) {
    dl->color = {1.0f, 1.0f, 1.0f, 1.0f};
    dl->direction = Normalize({0.55f, -0.45f, 0.65f}); // 斜光で雲海の凹凸を強調
    dl->intensity = 1.05f;
  }

  // ポストプロセス設定
  if (postProcess_) {
    // 被写界深度 (DoF): ドラゴンにフォーカス
    postProcess_->SetPostEffectType(7); // 7: Depth of Field
    postProcess_->SetDofFocusDistance(5.5f);
    postProcess_->SetDofFocusRange(8.0f);

    // ブルーム設定
    postProcess_->SetUseBloom(true);
    postProcess_->SetBloomIntensity(0.55f);
    postProcess_->SetBloomThreshold(0.92f);
    postProcess_->SetBloomSigma(3.0f);

    // ビネット無効化
    postProcess_->SetUseVignette(false);

    // トーンマッピング・露出設定
    postProcess_->SetToneMappingType(0);
    postProcess_->SetExposure(1.05f);
  }

  // フォグ設定
  FogData fog;
  fog.color = Vector4(0.72f, 0.86f, 1.0f, 1.0f);
  fog.nearDist = 300.0f;
  fog.farDist = 2800.0f;
  fog.enabled = 1.0f;
  engine_->GetObject3dRenderer()->SetFog(fog);

  //===========================
  // SkyBoxの初期化
  //===========================
  TextureManager::GetInstance()->LoadTexture("resources/Skybox/Skybox.dds");
  skybox_ = std::make_unique<Skybox>();
  skybox_->Initialize(engine_->GetSkyboxRenderer());
  skybox_->SetTexture("resources/Skybox/Skybox.dds");
  skybox_->SetScale({100.0f, 100.0f, 100.0f});
  skybox_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

  //===========================
  // 3Dオブジェクト関係の初期化
  //===========================
  ModelManager::GetInstance()->LoadModel("player_dragon.obj");
  ModelManager::GetInstance()->LoadModel("sea_of_clouds.obj");

  // 自機ドラゴンオブジェクト
  dragonObject_ = std::make_unique<Object3d>();
  dragonObject_->Initialize(engine_->GetObject3dRenderer());
  dragonObject_->SetModel("player_dragon.obj");
  dragonObject_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
  baseDragonPos_ = {0.0f, 0.0f, 0.0f};
  baseDragonRot_ = {0.0f, 0.0f, 0.0f};
  dragonTransform_.scale = {1.45f, 1.45f, 1.45f};
  dragonTransform_.rotate = baseDragonRot_;
  dragonTransform_.translate = baseDragonPos_;
  dragonObject_->SetScale(dragonTransform_.scale);
  dragonObject_->SetRotation(dragonTransform_.rotate);
  dragonObject_->SetTranslation(dragonTransform_.translate);

  // 雲海オブジェクト（手前・奥の2枚構成でループスクロール）
  cloudsObject_ = std::make_unique<Object3d>();
  cloudsObject_->Initialize(engine_->GetObject3dRenderer());
  cloudsObject_->SetModel("sea_of_clouds.obj");
  cloudsObject_->SetScale({6000.0f, 3000.0f, 1.0f});
  cloudsObject_->SetRotation({-1.5708f, 0.0f, 0.0f});
  cloudsObject_->SetTranslation({0.0f, -15.0f, 1500.0f});
  cloudsObject_->SetColor({0.88f, 0.93f, 0.98f, 1.0f});

  cloudsObjectFar_ = std::make_unique<Object3d>();
  cloudsObjectFar_->Initialize(engine_->GetObject3dRenderer());
  cloudsObjectFar_->SetModel("sea_of_clouds.obj");
  cloudsObjectFar_->SetScale({6000.0f, 3000.0f, 1.0f});
  cloudsObjectFar_->SetRotation({-1.5708f, 0.0f, 0.0f});
  cloudsObjectFar_->SetTranslation({0.0f, -15.0f, 4500.0f});
  cloudsObjectFar_->SetColor({0.88f, 0.93f, 0.98f, 1.0f});

  // カメラの生成と初期化
  camera_ = std::make_unique<GameCamera>();
  camera_->SetRotate({0.08f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 0.9f, -5.5f});
  camera_->SetFovY(0.70f);

  // デバッグカメラ
  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize({0.0f, 0.9f, -5.5f});

  // デフォルトカメラのセット
  engine_->GetObject3dRenderer()->SetDefaultCamera(camera_.get());

  //===========================
  // パーティクル関係の初期化
  //===========================

  // UIの読み込み
  UIManager::GetInstance()->Load("resources/UI/TitleUI.json");
}

void TitleScene::Finalize() {
  if (postProcess_) {
    postProcess_->SetUseRadialBlur(false);
  }
}

void TitleScene::Update() {

  // Sound更新
  SoundManager::GetInstance()->Update();

  // ステージセレクトシーンへ移行（発進演出トリガー）
  if (GameManager::GetInstance()->IsGlobalPlayMode()) {
    if (!isStarting_ && engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
      isStarting_ = true;
      startTimer_ = 0.0f;
      startCut_ = currentCut_; // 発進時のカメラカットを記録
      if (camera_) {
        startCamPos_ = camera_->GetTranslate();
        startCamRot_ = camera_->GetRotate();
      }
      if (currentCut_ == TitleCameraCut::FrontTracking) {
        startCamFov_ = 0.65f;
      } else if (currentCut_ == TitleCameraCut::OverTheWing) {
        startCamFov_ = 0.72f;
      } else {
        startCamFov_ = 0.70f;
      }
      startDragonPos_ = dragonTransform_.translate;
      startDragonRot_ = dragonTransform_.rotate;
      hasTransitioned_ = false;
      SoundManager::GetInstance()->PlaySE("ui_decide");
    }
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
  Input *input = GameManager::GetInstance()->IsGlobalPlayMode()
                     ? engine_->GetInputManager()
                     : nullptr;
  UIManager::GetInstance()->Update(input);

  // 発進演出中はUIをフェードアウト
  float uiFadeAlpha = 1.0f;
  if (isStarting_) {
    uiFadeAlpha = (std::max)(0.0f, 1.0f - startTimer_ / 0.30f);
  }

  if (auto titleLogoNode = UIManager::GetInstance()->GetNodeByName("TitleLogo")) {
    titleLogoNode->color = Vector4(1.0f, 1.0f, 1.0f, uiFadeAlpha);
  }

  // スタートテキストの呼吸明滅（サイン波アニメーション）
  if (auto startTextNode = UIManager::GetInstance()->GetNodeByName("StartText")) {
    float breathAlpha = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(motionTimer_ * 3.5f));
    startTextNode->color = Vector4(0.05f, 0.45f, 0.95f, breathAlpha * uiFadeAlpha);
  }

  //=======================
  // 3Dオブジェクトとカメラの更新
  //=======================
  Vector3 targetCamPos = {0.0f, 0.9f, -5.5f};
  Vector3 targetCamRot = {0.08f, 0.0f, 0.0f};

  // ターゲットに向ける注視回転角（ピッチ・ヨー）の計算ラムダ
  auto CalcLookAtRot = [](const Vector3 &eye, const Vector3 &target) -> Vector3 {
    Vector3 dir = target - eye;
    float distXZ = std::sqrt(dir.x * dir.x + dir.z * dir.z);
    float pitch = -std::atan2(dir.y, (std::max)(distXZ, 0.001f));
    float yaw = std::atan2(dir.x, dir.z);
    return {pitch, yaw, 0.0f};
  };

  if (isStarting_) {
    //==========================================
    // 発進演出処理（カメラ・ドラゴンの加速とシーン遷移同期）
    //==========================================
    startTimer_ += 1.0f / 60.0f;
    float progress = (std::clamp)(startTimer_ / kStartDuration_, 0.0f, 1.0f);

    // 加速カーブ（3乗イージング）
    float accelCurve = progress * progress * progress;
    // ホワイトアウト完了まで最高速の前進を維持
    float extraTime = (std::max)(0.0f, startTimer_ - kStartDuration_);
    float accelDist = accelCurve * 260.0f + extraTime * 300.0f;

    // 前傾姿勢への補間（SmoothStep）
    float poseT = (std::clamp)(progress / 0.40f, 0.0f, 1.0f);
    float smoothPose = poseT * poseT * (3.0f - 2.0f * poseT);

    // 雲海スクロール加速
    float scrollAccel = accelCurve * 120.0f + extraTime * 150.0f;
    cloudsScrollZ_ -= (3.0f + scrollAccel);
    if (cloudsScrollZ_ <= -3000.0f) {
      cloudsScrollZ_ += 3000.0f;
    }
    if (cloudsObject_) {
      cloudsObject_->SetTranslation({0.0f, -15.0f, 1500.0f + cloudsScrollZ_});
      cloudsObject_->Update();
    }
    if (cloudsObjectFar_) {
      cloudsObjectFar_->SetTranslation({0.0f, -15.0f, 4500.0f + cloudsScrollZ_});
      cloudsObjectFar_->Update();
    }

    switch (startCut_) {
    case TitleCameraCut::RearWide: {
      // カット1（後方）：背後追従しながら前傾姿勢で加速
      if (dragonObject_) {
        dragonTransform_.translate = startDragonPos_ + Vector3{0.0f, 0.15f * progress, accelDist};
        dragonTransform_.rotate = Lerp(startDragonRot_, Vector3{0.14f, 0.0f, 0.0f}, smoothPose);
        dragonObject_->SetTranslation(dragonTransform_.translate);
        dragonObject_->SetRotation(dragonTransform_.rotate);
        dragonObject_->Update();
      }

      // カメラを背後至近距離へ詰めて追従
      Vector3 initialOffset = startCamPos_ - startDragonPos_;
      Vector3 targetOffset = Vector3{0.0f, 0.65f, -3.8f};
      Vector3 currentOffset = Lerp(initialOffset, targetOffset, smoothPose);
      targetCamPos = dragonTransform_.translate + currentOffset;
      targetCamRot = Lerp(startCamRot_, Vector3{0.06f, 0.0f, 0.0f}, smoothPose);

      if (camera_) {
        camera_->SetFovY(Lerp(startCamFov_, 0.88f, accelCurve));
      }
      break;
    }
    case TitleCameraCut::FrontTracking: {
      // カット2（正面）：カメラ位置を保持し、ドラゴンの前進通過を注視追尾
      float flybyDist = accelCurve * 110.0f + extraTime * 150.0f;

      if (dragonObject_) {
        dragonTransform_.translate = startDragonPos_ + Vector3{0.0f, 0.20f * progress, flybyDist};
        dragonTransform_.rotate = Lerp(startDragonRot_, Vector3{0.14f, 0.0f, 0.0f}, smoothPose);
        dragonObject_->SetTranslation(dragonTransform_.translate);
        dragonObject_->SetRotation(dragonTransform_.rotate);
        dragonObject_->Update();
      }

      // カメラは初期位置でドラゴンの通過を注視
      targetCamPos = startCamPos_ + Vector3{0.0f, 0.10f * progress, 0.0f};
      Vector3 lookTarget = dragonTransform_.translate + Vector3{0.0f, 0.2f, 0.0f};
      Vector3 desiredRot = CalcLookAtRot(targetCamPos, lookTarget);
      targetCamRot = Lerp(startCamRot_, desiredRot, smoothPose);

      if (camera_) {
        camera_->SetFovY(Lerp(startCamFov_, 0.80f, accelCurve));
      }
      break;
    }
    case TitleCameraCut::OverTheWing: {
      // カット3（翼越し）：バンク角を保ったまま同調加速
      if (dragonObject_) {
        dragonTransform_.translate = startDragonPos_ + Vector3{0.0f, 0.15f * progress, accelDist};
        Vector3 targetRot = Vector3{0.12f, 0.0f, startDragonRot_.z * 0.6f};
        dragonTransform_.rotate = Lerp(startDragonRot_, targetRot, smoothPose);
        dragonObject_->SetTranslation(dragonTransform_.translate);
        dragonObject_->SetRotation(dragonTransform_.rotate);
        dragonObject_->Update();
      }

      // 翼越し相対オフセットを維持して追従
      Vector3 wingOffset = startCamPos_ - startDragonPos_;
      targetCamPos = dragonTransform_.translate + wingOffset;
      Vector3 lookTarget = dragonTransform_.translate + Vector3{0.6f, 0.05f, 10.0f};
      Vector3 desiredRot = CalcLookAtRot(targetCamPos, lookTarget);
      desiredRot.z = dragonTransform_.rotate.z * 0.45f;
      targetCamRot = Lerp(startCamRot_, desiredRot, smoothPose);

      if (camera_) {
        camera_->SetFovY(Lerp(startCamFov_, 0.88f, accelCurve));
      }
      break;
    }
    }

    // DoF
    if (postProcess_) {
      postProcess_->SetDofFocusDistance(5.0f);
      postProcess_->SetDofFocusRange(15.0f);
    }

    // ラジアルブラー制御（加速に応じて強度調整）
    if (postProcess_) {
      if (progress >= 0.20f) {
        postProcess_->SetUseRadialBlur(true);
        float blurProgress = (progress - 0.20f) / 0.80f;
        float blurWidth = blurProgress * blurProgress * 0.060f;
        postProcess_->SetRadialBlurCenter(0.5f, 0.5f);
        postProcess_->SetRadialBlurWidth(blurWidth);
        postProcess_->SetRadialBlurInnerRadius(0.06f);
        postProcess_->SetRadialBlurOuterRadius(1.0f);
        postProcess_->SetRadialBlurSamples(8);
      }
    }

    // 加速ピーク（1.30s）に合わせて0.85s時点で白フェードアウトを開始
    if (startTimer_ >= 0.85f && !hasTransitioned_) {
      hasTransitioned_ = true;
      SceneManager::GetInstance()->SetNextTransitionFade(
          0.45f, Fade::FadeType::Solid, Vector4{1.0f, 1.0f, 1.0f, 1.0f});
      SceneManager::GetInstance()->ChangeScene("STAGE_SELECT");
    }

  } else {
    //==========================================
    // 通常鑑賞モード（プロシージャル飛行とカメラ巡回）
    //==========================================
    motionTimer_ += 1.0f / 60.0f;

    // ドラゴンの待機フライトモーション（8の字旋回・バンク連動）
    if (dragonObject_) {
      float flightTime = motionTimer_ * 0.7f;

      // 8の字旋回
      float flightX = std::sin(flightTime) * 2.2f;
      float flightY = std::sin(flightTime * 2.0f) * 0.65f - 0.1f;
      float flightZ = std::cos(flightTime) * 0.8f;

      // 速度ベクトルから旋回角・ピッチを計算
      float vx = std::cos(flightTime) * 2.2f * 0.7f;
      float vy = 2.0f * std::cos(flightTime * 2.0f) * 0.65f * 0.7f;

      float roll = -vx * 0.28f;
      float pitch = -vy * 0.18f;
      float yaw = std::sin(flightTime) * 0.22f;

      // 羽ばたきの微小上下動
      float wingBeat = std::sin(motionTimer_ * 5.0f) * 0.08f;
      float wingPitch = std::cos(motionTimer_ * 5.0f) * 0.02f;

      dragonTransform_.translate = baseDragonPos_ + Vector3{flightX, flightY + wingBeat, flightZ};
      dragonTransform_.rotate = baseDragonRot_ + Vector3{pitch + wingPitch, yaw, roll};
      dragonObject_->SetTranslation(dragonTransform_.translate);
      dragonObject_->SetRotation(dragonTransform_.rotate);
      dragonObject_->Update();

      // 被写界深度（DoF）のオートフォーカス
      if (postProcess_) {
        Vector3 camPos = camera_ ? camera_->GetTranslate() : Vector3{0.0f, 0.9f, -5.5f};
        float distToDragon = Length(dragonTransform_.translate - camPos);
        postProcess_->SetDofFocusDistance(distToDragon);
        postProcess_->SetDofFocusRange(8.0f);
      }
    }

    // 雲海スクロール
    cloudsScrollZ_ -= 3.0f;
    if (cloudsScrollZ_ <= -3000.0f) {
      cloudsScrollZ_ += 3000.0f;
    }
    if (cloudsObject_) {
      cloudsObject_->SetTranslation({0.0f, -15.0f, 1500.0f + cloudsScrollZ_});
      cloudsObject_->Update();
    }
    if (cloudsObjectFar_) {
      cloudsObjectFar_->SetTranslation({0.0f, -15.0f, 4500.0f + cloudsScrollZ_});
      cloudsObjectFar_->Update();
    }

    // カメラアングルの自動切り替え
    if (!isManualCut_) {
      cutTimer_ += 1.0f / 60.0f;
      if (cutTimer_ >= kCutDuration_) {
        cutTimer_ = 0.0f;
        int nextCut = (static_cast<int>(currentCut_) + 1) % 3;
        currentCut_ = static_cast<TitleCameraCut>(nextCut);
      }
    }

    switch (currentCut_) {
    case TitleCameraCut::RearWide: {
      // カット1: 後方ワイド追従
      Vector3 drift = {std::sin(cutTimer_ * 0.4f) * 0.35f,
                       std::cos(cutTimer_ * 0.3f) * 0.15f, 0.0f};
      targetCamPos = Vector3{0.0f, 0.9f, -5.5f} + drift;
      Vector3 lookTarget = {dragonTransform_.translate.x * 0.3f,
                            dragonTransform_.translate.y * 0.2f,
                            dragonTransform_.translate.z + 1.5f};
      targetCamRot = CalcLookAtRot(targetCamPos, lookTarget);
      if (camera_) {
        camera_->SetFovY(0.70f);
      }
      break;
    }
    case TitleCameraCut::FrontTracking: {
      // カット2: 斜め前方並走
      Vector3 baseOffset = {1.9f, 0.25f, 3.8f};
      Vector3 drift = {std::cos(cutTimer_ * 0.45f) * 0.2f,
                       std::sin(cutTimer_ * 0.35f) * 0.15f, -cutTimer_ * 0.05f};
      targetCamPos = dragonTransform_.translate + baseOffset + drift;
      Vector3 lookTarget = dragonTransform_.translate + Vector3{0.0f, 0.1f, 0.0f};
      targetCamRot = CalcLookAtRot(targetCamPos, lookTarget);
      if (camera_) {
        camera_->SetFovY(0.65f);
      }
      break;
    }
    case TitleCameraCut::OverTheWing: {
      // カット3: 翼越し（ロール連動）
      Vector3 baseOffset = {-1.65f, 0.45f, -1.85f};
      Vector3 drift = {std::sin(cutTimer_ * 0.35f) * 0.12f,
                       std::cos(cutTimer_ * 0.25f) * 0.08f, cutTimer_ * 0.06f};
      targetCamPos = dragonTransform_.translate + baseOffset + drift;

      Vector3 lookTarget = dragonTransform_.translate + Vector3{0.6f, 0.05f, 10.0f};
      targetCamRot = CalcLookAtRot(targetCamPos, lookTarget);
      targetCamRot.z = dragonTransform_.rotate.z * 0.45f;

      if (camera_) {
        camera_->SetFovY(0.72f);
      }
      break;
    }
    }
  }

  // カメラへ反映
  if (camera_ && !useDebugCamera_) {
    camera_->SetTranslate(targetCamPos);
    camera_->SetRotate(targetCamRot);
  }

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

void TitleScene::Draw() { Draw3D(); }

void TitleScene::Draw3D() {
  engine_->Begin3D();

  // Skybox描画
  if (skybox_) {
    engine_->GetSkyboxRenderer()->Begin();
    skybox_->Draw();

    // 通常3Dオブジェクトレンダラーへ復帰
    engine_->GetObject3dRenderer()->Begin();
  }

  // 雲海描画
  if (cloudsObject_) {
    cloudsObject_->Draw();
  }
  if (cloudsObjectFar_) {
    cloudsObjectFar_->Draw();
  }

  // 自機ドラゴン描画
  if (dragonObject_) {
    dragonObject_->Draw();
  }
}

void TitleScene::Draw2D() {
  // ここから下で2DオブジェクトのDrawを呼ぶ
  UIManager::GetInstance()->Draw();
}

void TitleScene::DrawEditorUI() {
#ifdef USE_IMGUI
  ImGui::Begin("Title Scene Settings");
  if (ImGui::CollapsingHeader("Dragon Transform",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::DragFloat3("Base Pos", &baseDragonPos_.x, 0.1f);
    ImGui::DragFloat3("Base Rot", &baseDragonRot_.x, 0.01f);
  }
  if (camera_ &&
      ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    Vector3 camPos = camera_->GetTranslate();
    Vector3 camRot = camera_->GetRotate();
    if (ImGui::DragFloat3("Cam Pos", &camPos.x, 0.1f)) {
      camera_->SetTranslate(camPos);
    }
    if (ImGui::DragFloat3("Cam Rot", &camRot.x, 0.01f)) {
      camera_->SetRotate(camRot);
    }
  }
  if (postProcess_ &&
      ImGui::CollapsingHeader("PostProcess / DoF", ImGuiTreeNodeFlags_DefaultOpen)) {
    float dofDist = postProcess_->GetDofFocusDistance();
    float dofRange = postProcess_->GetDofFocusRange();
    if (ImGui::DragFloat("DoF Focus Distance", &dofDist, 0.1f, 0.0f, 50.0f)) {
      postProcess_->SetDofFocusDistance(dofDist);
    }
    if (ImGui::DragFloat("DoF Focus Range", &dofRange, 0.1f, 0.0f, 20.0f)) {
      postProcess_->SetDofFocusRange(dofRange);
    }
    float bloomThresh = postProcess_->GetBloomThreshold();
    if (ImGui::DragFloat("Bloom Threshold", &bloomThresh, 0.02f, 0.0f, 2.0f)) {
      postProcess_->SetBloomThreshold(bloomThresh);
    }
    float exposure = postProcess_->GetExposure();
    if (ImGui::DragFloat("Exposure", &exposure, 0.05f, 0.1f, 5.0f)) {
      postProcess_->SetExposure(exposure);
    }
  }
  if (ImGui::CollapsingHeader("Cinematic Camera Cuts", ImGuiTreeNodeFlags_DefaultOpen)) {
    const char *cutNames[] = {"Rear Wide (Main)", "Front Tracking (Close-up)", "Over-The-Wing (Cockpit View)"};
    int cutIndex = static_cast<int>(currentCut_);
    if (ImGui::Combo("Active Cut", &cutIndex, cutNames, IM_ARRAYSIZE(cutNames))) {
      currentCut_ = static_cast<TitleCameraCut>(cutIndex);
      cutTimer_ = 0.0f;
    }
    ImGui::Checkbox("Manual Cut (Pause Auto Switch)", &isManualCut_);
    float progress = cutTimer_ / kCutDuration_;
    ImGui::ProgressBar(progress, ImVec2(-1, 0), "Cut Progress");
  }
  ImGui::End();
#endif
}