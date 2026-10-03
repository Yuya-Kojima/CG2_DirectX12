#include "TitleScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Framework/GameManager.h"
#include "Framework/UIManager.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Render/SkyBox/SkyBox.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/PostProcess.h"
#include "Renderer/TrailRenderer.h"
#include "Scene/SceneManager.h"
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
  TextureManager::GetInstance()->LoadTexture("resources/gradationLine.png");

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
  windStreaks_.clear();
  windStreaks_.reserve(kMaxWindStreaks_);
  for (size_t i = 0; i < kMaxWindStreaks_; ++i) {
    WindStreak streak{};
    // ドラゴンとカメラの視界内（幅±4.5m、高さ±1.5m、奥行き-4m〜22m）に集中配置
    float rx = (static_cast<float>(rand()) / RAND_MAX) * 9.0f - 4.5f;
    float ry = (static_cast<float>(rand()) / RAND_MAX) * 3.2f - 1.2f;
    float rz = (static_cast<float>(rand()) / RAND_MAX) * 26.0f - 4.0f;
    streak.pos = {rx, ry, rz};
    streak.baseLength = 7.0f + (static_cast<float>(rand()) / RAND_MAX) * 6.0f;  // 7m〜13mの流線
    streak.speed = 26.0f + (static_cast<float>(rand()) / RAND_MAX) * 14.0f;     // 秒速26〜40mの疾走感
    streak.alpha = 0.60f + (static_cast<float>(rand()) / RAND_MAX) * 0.35f;
    streak.width = 0.025f + (static_cast<float>(rand()) / RAND_MAX) * 0.015f;   // 2.5〜4.0cmのクッキリ見える風筋
    windStreaks_.push_back(streak);
  }

  // UIの読み込み
  UIManager::GetInstance()->Load("resources/UI/TitleUI.json");
}

void TitleScene::Finalize() {
  if (postProcess_) {
    postProcess_->SetUseRadialBlur(false);
  }
}

Vector3 TitleScene::CalcLookAtRot(const Vector3 &eye, const Vector3 &target) {
  Vector3 dir = target - eye;
  float distXZ = std::sqrt(dir.x * dir.x + dir.z * dir.z);
  float pitch = -std::atan2(dir.y, (std::max)(distXZ, 0.001f));
  float yaw = std::atan2(dir.x, dir.z);
  return {pitch, yaw, 0.0f};
}

void TitleScene::UpdateUI() {
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

  // スプライト・UIの更新
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
  float breathAlpha = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(motionTimer_ * 3.5f));

  // シャドウの更新（濃い黒、フェード連動）
  if (auto shadowNode = UIManager::GetInstance()->GetNodeByName("StartTextShadow")) {
    shadowNode->color = Vector4(0.0f, 0.0f, 0.0f, 0.85f * breathAlpha * uiFadeAlpha);
  }

  // 本体の更新（鮮やかな青、フェード連動）
  if (auto startTextNode = UIManager::GetInstance()->GetNodeByName("StartText")) {
    startTextNode->color = Vector4(0.05f, 0.45f, 0.95f, breathAlpha * uiFadeAlpha);
  }
}

void TitleScene::UpdateLaunchSequence(Vector3 &outTargetCamPos, Vector3 &outTargetCamRot, float &outLaunchAccelCurve) {
  startTimer_ += 1.0f / 60.0f;
  float progress = (std::clamp)(startTimer_ / kStartDuration_, 0.0f, 1.0f);

  // 加速カーブ（3乗イージング）
  float accelCurve = progress * progress * progress;
  outLaunchAccelCurve = accelCurve;
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
  }
  if (cloudsObjectFar_) {
    cloudsObjectFar_->SetTranslation({0.0f, -15.0f, 4500.0f + cloudsScrollZ_});
  }

  switch (startCut_) {
  case TitleCameraCut::RearWide: {
    // カット1（後方）：背後追従しながら前傾姿勢で加速
    dragonTransform_.translate = startDragonPos_ + Vector3{0.0f, 0.15f * progress, accelDist};
    dragonTransform_.rotate = Lerp(startDragonRot_, Vector3{0.14f, 0.0f, 0.0f}, smoothPose);

    // 通常時の見やすい距離（約 -5.5m）を基準に、発進前傾姿勢に合わせてオフセットを調整
    Vector3 initialOffset = startCamPos_ - startDragonPos_;
    Vector3 readyOffset = Vector3{0.0f, 0.85f, -5.5f};
    Vector3 currentOffset = Lerp(initialOffset, readyOffset, smoothPose);

    // 加速によるカメラの引き離され演出（ドラゴンの急加速にカメラが一瞬遅れて追従することで飛び出し感を表現）
    float lagDistance = accelCurve * 3.5f; // 最大で約3.5m後方に引き離される（距離が約 -5.5m -> -9.0m へ）
    currentOffset.z -= lagDistance;

    outTargetCamPos = dragonTransform_.translate + currentOffset;
    outTargetCamRot = Lerp(startCamRot_, Vector3{0.06f, 0.0f, 0.0f}, smoothPose);

    if (camera_) {
      camera_->SetFovY(Lerp(startCamFov_, 0.88f, accelCurve));
    }
    break;
  }
  case TitleCameraCut::FrontTracking: {
    // カット2（正面）：カメラ位置を保持し、ドラゴンの前進通過を注視追尾
    float flybyDist = accelCurve * 110.0f + extraTime * 150.0f;

    dragonTransform_.translate = startDragonPos_ + Vector3{0.0f, 0.20f * progress, flybyDist};
    dragonTransform_.rotate = Lerp(startDragonRot_, Vector3{0.14f, 0.0f, 0.0f}, smoothPose);

    // カメラは初期位置でドラゴンの通過を注視
    outTargetCamPos = startCamPos_ + Vector3{0.0f, 0.10f * progress, 0.0f};
    Vector3 lookTarget = dragonTransform_.translate + Vector3{0.0f, 0.2f, 0.0f};
    Vector3 desiredRot = CalcLookAtRot(outTargetCamPos, lookTarget);
    outTargetCamRot = Lerp(startCamRot_, desiredRot, smoothPose);

    if (camera_) {
      camera_->SetFovY(Lerp(startCamFov_, 0.80f, accelCurve));
    }
    break;
  }
  case TitleCameraCut::OverTheWing: {
    // カット3（翼越し）：バンク角を保ったまま同調加速
    dragonTransform_.translate = startDragonPos_ + Vector3{0.0f, 0.15f * progress, accelDist};
    Vector3 targetRot = Vector3{0.12f, 0.0f, startDragonRot_.z * 0.6f};
    dragonTransform_.rotate = Lerp(startDragonRot_, targetRot, smoothPose);

    // 翼越し相対オフセットを維持しながら、加速に合わせて少し後方に引かれる演出
    Vector3 wingOffset = startCamPos_ - startDragonPos_;
    float lagDistance = accelCurve * 1.0f;
    wingOffset.z -= lagDistance;

    outTargetCamPos = dragonTransform_.translate + wingOffset;
    Vector3 lookTarget = dragonTransform_.translate + Vector3{0.6f, 0.05f, 10.0f};
    Vector3 desiredRot = CalcLookAtRot(outTargetCamPos, lookTarget);
    desiredRot.z = dragonTransform_.rotate.z * 0.45f;
    outTargetCamRot = Lerp(startCamRot_, desiredRot, smoothPose);

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
}

void TitleScene::UpdateIdleMotion(Vector3 &outTargetCamPos, Vector3 &outTargetCamRot) {
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
  }

  // 雲海スクロール
  cloudsScrollZ_ -= 3.0f;
  if (cloudsScrollZ_ <= -3000.0f) {
    cloudsScrollZ_ += 3000.0f;
  }
  if (cloudsObject_) {
    cloudsObject_->SetTranslation({0.0f, -15.0f, 1500.0f + cloudsScrollZ_});
  }
  if (cloudsObjectFar_) {
    cloudsObjectFar_->SetTranslation({0.0f, -15.0f, 4500.0f + cloudsScrollZ_});
  }

  // カメラアングルの自動切り替え
  if (!isManualCut_) {
    cutTimer_ += 1.0f / 60.0f;
    if (cutTimer_ >= kCutDuration_) {
      cutTimer_ = 0.0f;
      int nextCut = (static_cast<int>(currentCut_) + 1) % 3;
      currentCut_ = static_cast<TitleCameraCut>(nextCut);
      rightTrailHistory_.clear();
      leftTrailHistory_.clear();
    }
  }

  switch (currentCut_) {
  case TitleCameraCut::RearWide: {
    // カット1: 後方ワイド追従
    Vector3 drift = {std::sin(cutTimer_ * 0.4f) * 0.35f,
                     std::cos(cutTimer_ * 0.3f) * 0.15f, 0.0f};
    outTargetCamPos = Vector3{0.0f, 0.9f, -5.5f} + drift;
    Vector3 lookTarget = {dragonTransform_.translate.x * 0.3f,
                          dragonTransform_.translate.y * 0.2f,
                          dragonTransform_.translate.z + 1.5f};
    outTargetCamRot = CalcLookAtRot(outTargetCamPos, lookTarget);
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
    outTargetCamPos = dragonTransform_.translate + baseOffset + drift;
    Vector3 lookTarget = dragonTransform_.translate + Vector3{0.0f, 0.1f, 0.0f};
    outTargetCamRot = CalcLookAtRot(outTargetCamPos, lookTarget);
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
    outTargetCamPos = dragonTransform_.translate + baseOffset + drift;

    Vector3 lookTarget = dragonTransform_.translate + Vector3{0.6f, 0.05f, 10.0f};
    outTargetCamRot = CalcLookAtRot(outTargetCamPos, lookTarget);
    outTargetCamRot.z = dragonTransform_.rotate.z * 0.45f;

    if (camera_) {
      camera_->SetFovY(0.72f);
    }
    break;
  }
  }
}

void TitleScene::UpdateLighting() {
  // シネマティックカメラカットに連動したライトの向き制御
  TitleCameraCut activeCut = isStarting_ ? startCut_ : currentCut_;
  Vector3 targetLightDir = {0.55f, -0.45f, 0.65f};
  if (activeCut == TitleCameraCut::FrontTracking) {
    // カット2（正面・並走）：斜め前方上空から顔・胸元を美しく照らして逆光を防ぐ
    targetLightDir = {-0.45f, -0.55f, -0.70f};
  }
  targetLightDir = Normalize(targetLightDir);

  // カット切り替え時に滑らかに光の向きを補間
  currentLightDir_ = Normalize(Lerp(currentLightDir_, targetLightDir, 0.10f));

  if (auto *dl = engine_->GetObject3dRenderer()->GetDirectionalLightData()) {
    dl->direction = currentLightDir_;
  }
}

const ICamera *TitleScene::UpdateActiveCamera(const Vector3 &targetCamPos, const Vector3 &targetCamRot) {
  // デバッグカメラ切り替え
  if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
    useDebugCamera_ = !useDebugCamera_;
  }

  // カメラへ目標位置・回転を反映
  if (camera_ && !useDebugCamera_) {
    camera_->SetTranslate(targetCamPos);
    camera_->SetRotate(targetCamRot);
  }

  // カメラの更新
  const ICamera *activeCamera = nullptr;
  if (useDebugCamera_) {
    debugCamera_->Update(*engine_->GetInputManager());
    activeCamera = debugCamera_->GetCamera();
  } else {
    camera_->Update();
    activeCamera = camera_.get();
  }

  // アクティブカメラを描画システムへセット
  engine_->GetObject3dRenderer()->SetDefaultCamera(activeCamera);

  return activeCamera;
}

void TitleScene::Update3DObjects(const ICamera *activeCamera) {
  // 最新のカメラWVP行列を適用するためカメラ確定後に実行
  if (dragonObject_) {
    dragonObject_->SetTranslation(dragonTransform_.translate);
    dragonObject_->SetRotation(dragonTransform_.rotate);
    dragonObject_->Update();
  }

  if (cloudsObject_) {
    cloudsObject_->Update();
  }
  if (cloudsObjectFar_) {
    cloudsObjectFar_->Update();
  }

  // 被写界深度（DoF）のオートフォーカス（通常時）
  if (!isStarting_ && postProcess_ && activeCamera) {
    float distToDragon = Length(dragonTransform_.translate - activeCamera->GetTranslate());
    postProcess_->SetDofFocusDistance(distToDragon);
    postProcess_->SetDofFocusRange(8.0f);
  }
}

void TitleScene::UpdateWingTrails(const ICamera *activeCamera, float launchAccelCurve) {
  if (!dragonObject_ || !activeCamera) {
    return;
  }

  // ドラゴンの翼端ローカル座標（player_dragon.objの先端頂点）
  const Vector3 localRightTip = {0.498f, 0.375f, 0.097f};
  const Vector3 localLeftTip = {-0.498f, 0.375f, 0.097f};

  // ワールド行列から翼端の現在位置を算出
  Vector3 currentRightPos = TransformPoint(localRightTip, dragonObject_->GetWorldMatrix());
  Vector3 currentLeftPos = TransformPoint(localLeftTip, dragonObject_->GetWorldMatrix());

  // 相対風速（通常巡航時はドラゴンの体長に合わせた流速、発進加速の進捗に合わせて空間固定へスムーズに移行）
  float windShift = trailWindShift_;
  if (isStarting_) {
    // 溜め期間（姿勢変更中）は自然に後方へ流し続け、加速本格化に合わせて空間固定へ移行
    windShift = trailWindShift_ * (1.0f - launchAccelCurve);
  }

  // 過去の軌跡ノードを後方（-Z方向）へ移動
  for (auto &pos : rightTrailHistory_) {
    pos.z -= windShift;
  }
  for (auto &pos : leftTrailHistory_) {
    pos.z -= windShift;
  }

  // 翼端ノードの追加（発進加速時は高速移動に合わせてサブステップ補間）
  auto AppendTrailNode = [this](std::deque<Vector3> &history, const Vector3 &newPos) {
    if (history.empty()) {
      history.push_front(newPos);
      return;
    }

    if (isStarting_) {
      // 発進加速時：フレーム間の移動量が数メートルに達するため、0.25m刻みで補間ノードを挿入
      Vector3 prevPos = history.front();
      Vector3 delta = newPos - prevPos;
      float dist = Length(delta);
      const float kStepDist = 0.25f;
      if (dist > kStepDist) {
        int steps = static_cast<int>(std::ceil(dist / kStepDist));
        steps = (std::min)(steps, 24); // 上限クリップ
        for (int s = 1; s <= steps; ++s) {
          float t = static_cast<float>(s) / static_cast<float>(steps);
          history.push_front(prevPos + delta * t);
        }
      } else {
        history.push_front(newPos);
      }
    } else {
      // 通常鑑賞モード：1フレーム1ノード
      history.push_front(newPos);
    }
  };

  AppendTrailNode(rightTrailHistory_, currentRightPos);
  AppendTrailNode(leftTrailHistory_, currentLeftPos);

  size_t maxHistory = isStarting_ ? 256 : kMaxTrailHistory_;
  while (rightTrailHistory_.size() > maxHistory) {
    rightTrailHistory_.pop_back();
  }
  while (leftTrailHistory_.size() > maxHistory) {
    leftTrailHistory_.pop_back();
  }

  // 発進加速時：一定以上後方に離れた（尾を引きすぎた）古いノードのみ自然に消去
  if (isStarting_) {
    float minZ = currentRightPos.z - 28.0f;
    while (!rightTrailHistory_.empty() && rightTrailHistory_.back().z < minZ) {
      rightTrailHistory_.pop_back();
    }
    while (!leftTrailHistory_.empty() && leftTrailHistory_.back().z < minZ) {
      leftTrailHistory_.pop_back();
    }
  }

  // トレイルノード列の構築（2点以上あれば生成）
  auto BuildNodes = [this, launchAccelCurve](const std::deque<Vector3> &history) {
    std::vector<TrailRenderer::TrailNode> nodes;
    if (!enableTrail_ || history.size() < 2) {
      return nodes;
    }

    size_t count = history.size();
    nodes.reserve(count);

    for (size_t i = 0; i < count; ++i) {
      float t = static_cast<float>(i) / static_cast<float>(count - 1); // 0.0(根本) -> 1.0(末尾)

      float currentWidth = trailWidth_;
      float currentAlpha = trailAlpha_;
      if (isStarting_) {
        // 発進加速の進行度（launchAccelCurve）に合わせて滑らかに強調（Enter直後の急変を防止）
        currentWidth *= (1.0f + 0.25f * launchAccelCurve);
        currentAlpha = (std::min)(1.0f, currentAlpha * (1.0f + 0.20f * launchAccelCurve));
      }

      // 太さ: 翼端の付け根は適度に絞り、中間で最大幅、末尾で0に先細り
      float width = currentWidth * (1.0f - t * 0.85f);
      if (i == 0) {
        width *= 0.35f;
      }

      // アルファ: 線形フェードアウト（背景の雲海・青空でもしっかりと視認可能）
      float alpha = (1.0f - t) * currentAlpha;

      // 色: コアが爽やかに光るライトシアン（ブルームと自然に調和）
      Vector4 color = {0.85f * alpha, 0.95f * alpha, 1.15f * alpha, alpha};

      nodes.push_back({history[i], width, color});
    }
    return nodes;
  };

  auto rightNodes = BuildNodes(rightTrailHistory_);
  auto leftNodes = BuildNodes(leftTrailHistory_);

  Vector3 camPos = activeCamera->GetTranslate();
  if (!rightNodes.empty()) {
    TrailRenderer::GetInstance()->AddTrail(rightNodes, camPos);
  }
  if (!leftNodes.empty()) {
    TrailRenderer::GetInstance()->AddTrail(leftNodes, camPos);
  }
}

void TitleScene::UpdateWindStreaks(const ICamera *activeCamera) {
  if (!enableWind_ || !activeCamera) {
    return;
  }

  Vector3 camPos = activeCamera->GetTranslate();
  Vector3 dragonPos = dragonTransform_.translate;
  float dt = 1.0f / 60.0f;

  // 発進演出に合わせた流速と長さのブースト
  float boostSpeed = 0.0f;
  float boostLen = 1.0f;
  if (isStarting_) {
    float progress = (std::clamp)(startTimer_ / kStartDuration_, 0.0f, 1.0f);
    float accelCurve = progress * progress * progress;
    float extraTime = (std::max)(0.0f, startTimer_ - kStartDuration_);
    boostSpeed = accelCurve * 140.0f + extraTime * 180.0f;
    boostLen = 1.0f + accelCurve * 4.5f + extraTime * 6.0f;
  }

  for (auto &streak : windStreaks_) {
    float currentSpeed = (streak.speed + boostSpeed) * windSpeedMult_;
    streak.pos.z -= currentSpeed * dt;

    float curLen = streak.baseLength * windLengthMult_ * boostLen;

    // カメラ・ドラゴンの後方に抜けたら前方空間へ再配置
    float backBound = (std::min)(dragonPos.z, camPos.z) - 6.0f;
    if (streak.pos.z + curLen < backBound) {
      float rx = (static_cast<float>(rand()) / RAND_MAX) * 9.0f - 4.5f;
      float ry = (static_cast<float>(rand()) / RAND_MAX) * 3.2f - 1.2f;
      float rz = (std::max)(dragonPos.z, camPos.z) + 16.0f + (static_cast<float>(rand()) / RAND_MAX) * 12.0f;
      streak.pos = {rx, ry, rz};
    }

    // 2点ノード（先端と末尾）を構築
    Vector3 head = streak.pos;
    Vector3 tail = streak.pos + Vector3{0.0f, 0.0f, curLen};

    float a = streak.alpha * windAlpha_;

    // 色: クッキリと光るライトシアン（先端から末尾へフェードアウト）
    Vector4 headColor = {0.85f * a, 0.95f * a, 1.20f * a, a};
    Vector4 tailColor = {0.85f * a * 0.1f, 0.95f * a * 0.1f, 1.20f * a * 0.1f, 0.0f};

    std::vector<TrailRenderer::TrailNode> nodes;
    nodes.reserve(2);
    nodes.push_back({head, streak.width * 0.40f, headColor});
    nodes.push_back({tail, streak.width, tailColor});

    TrailRenderer::GetInstance()->AddTrail(nodes, camPos);
  }
}

void TitleScene::Update() {
  // Sound更新
  SoundManager::GetInstance()->Update();

  // 入力・UIの更新
  UpdateUI();

  // 演出計算（カメラ目標位置・回転、加速カーブ）
  Vector3 targetCamPos = {0.0f, 0.9f, -5.5f};
  Vector3 targetCamRot = {0.08f, 0.0f, 0.0f};
  float launchAccelCurve = 0.0f;

  if (isStarting_) {
    UpdateLaunchSequence(targetCamPos, targetCamRot, launchAccelCurve);
  } else {
    UpdateIdleMotion(targetCamPos, targetCamRot);
  }

  // カメラカット連動ライティングの更新
  UpdateLighting();

  // カメラ更新とアクティブカメラの確定
  const ICamera *activeCamera = UpdateActiveCamera(targetCamPos, targetCamRot);

  // 3Dオブジェクトの更新
  Update3DObjects(activeCamera);

  // エフェクトの更新
  UpdateWingTrails(activeCamera, launchAccelCurve);
  UpdateWindStreaks(activeCamera);
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

  // 自機描画
  if (dragonObject_) {
    dragonObject_->Draw();
  }

  // 翼端リボントレイル描画
  if (camera_) {
    auto srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("resources/gradationLine.png");
    TrailRenderer::GetInstance()->Render(camera_->GetViewProjectionMatrix(), srvHandle);
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
      rightTrailHistory_.clear();
      leftTrailHistory_.clear();
    }
    ImGui::Checkbox("Manual Cut (Pause Auto Switch)", &isManualCut_);
    float progress = cutTimer_ / kCutDuration_;
    ImGui::ProgressBar(progress, ImVec2(-1, 0), "Cut Progress");
  }
  if (ImGui::CollapsingHeader("Wingtip Trail Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Enable Trail", &enableTrail_);
    ImGui::SliderFloat("Trail Width", &trailWidth_, 0.01f, 0.50f, "%.3f");
    ImGui::SliderFloat("Trail Alpha", &trailAlpha_, 0.05f, 1.0f, "%.2f");
    ImGui::SliderFloat("Trail Speed (Length)", &trailWindShift_, 0.05f, 1.00f, "%.2f");
  }
  if (ImGui::CollapsingHeader("Wind Stream Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Enable Wind Stream", &enableWind_);
    ImGui::SliderFloat("Wind Speed Mult", &windSpeedMult_, 0.1f, 3.0f, "%.2f");
    ImGui::SliderFloat("Wind Length Mult", &windLengthMult_, 0.2f, 3.0f, "%.2f");
    ImGui::SliderFloat("Wind Alpha", &windAlpha_, 0.05f, 1.0f, "%.2f");
  }
  ImGui::End();
#endif
}