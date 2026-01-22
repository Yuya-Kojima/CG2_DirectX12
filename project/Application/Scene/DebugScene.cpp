#include "DebugScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Debug/ImGuiManager.h"
#include "Debug/Logger.h"
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
#include <fstream>

void DebugScene::Initialize(EngineBase *engine) {

  engine_ = engine;

  //===========================
  // テクスチャファイルの読み込み
  //===========================

  // テクスチャの読み込み
  TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

  TextureManager::GetInstance()->LoadTexture("resources/monsterball.png");

  //===========================
  // オーディオファイルの読み込み
  //===========================

  auto *sm = SoundManager::GetInstance();

  // Audioファイルを登録
  sm->Load("bgm_mokugyo", "resources/mokugyo.wav");
  sm->Load("se_fanfare", "resources/fanfare.wav");

  // bgm再生
  sm->PlayBGM("bgm_mokugyo");

  //===========================
  // スプライト関係の初期化
  //===========================

  // スプライトの生成と初期化
  for (int i = 0; i < kSpriteCount_; ++i) {
    Sprite *sprite = new Sprite();
    if (i % 2 == 1) {
      sprite->Initialize(engine_->GetSpriteRenderer(),
                         "resources/uvChecker.png");
    } else {
      sprite->Initialize(engine_->GetSpriteRenderer(),
                         "resources/uvChecker.png");
    }
    sprites_.push_back(sprite);
  }

  sprite_ = new Sprite();
  sprite_->Initialize(engine_->GetSpriteRenderer(), "resources/uvChecker.png");

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    spritePositions_[i] = {i * 200.0f, 0.0f};
    spriteSizes_[i] = {150.0f, 150.0f};
  }

  // スプライトのTransform
  spritePosition_ = {
      100.0f,
      100.0f,
  };

  spriteRotation_ = 0.0f;

  spriteColor_ = {1.0f, 1.0f, 1.0f, 1.0f};

  spriteSize_ = {640.0f, 360.0f};

  spriteAnchorPoint_ = {0.0f, 0.0f};

  sprite_->SetPosition(spritePosition_);
  sprite_->SetRotation(spriteRotation_);
  sprite_->SetColor(spriteColor_);
  sprite_->SetSize(spriteSize_);
  sprite_->SetAnchorPoint(spriteAnchorPoint_);
  sprite_->SetIsFlipX(isFlipX_);
  sprite_->SetIsFlipY(isFlipY_);

  // UVTransform用
  uvTransformSprite_ = {
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  //===========================
  // 3Dオブジェクト関係の初期化
  //===========================

  // カメラの生成と初期化
  camera_ = new GameCamera();
  camera_->SetRotate({0.3f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 4.0f, -10.0f});
  engine_->GetObject3dRenderer()->SetDefaultCamera(camera_);

  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f},
      {0.3f, 0.0f, 0.0f},
      {0.0f, 4.0f, -10.0f},
  };

  // デバッグカメラ
  debugCamera_ = new DebugCamera();
  debugCamera_->Initialize({0.0f, 4.0f, -10.0f});

  // モデルの読み込み
  ModelManager::GetInstance()->LoadModel("terrain.obj");

  ModelManager::GetInstance()->LoadModel("plane.gltf");

  // オブジェクトの生成と初期化
  object3d_ = new Object3d();
  object3d_->Initialize(engine_->GetObject3dRenderer());
  object3d_->SetModel("terrain.obj");

  object3dA_ = new Object3d();
  object3dA_->Initialize(engine_->GetObject3dRenderer());
  object3dA_->SetModel("plane.gltf");

  //===========================
  // パーティクル関係の初期化
  //===========================

  // グループ登録（name と texture を紐づけ）
  ParticleManager::GetInstance()->CreateParticleGroup("test",
                                                      "resources/circle.png");

  // エミッタ
  Transform emitterTransform{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 5.0f},
  };

  particleEmitter_ =
      new ParticleEmitter("test", emitterTransform, 3, 1.0f, 0.0f);
}

void DebugScene::Finalize() {
  delete object3dA_;
  object3dA_ = nullptr;

  delete object3d_;
  object3d_ = nullptr;

  delete debugCamera_;
  debugCamera_ = nullptr;

  delete camera_;
  camera_ = nullptr;

  delete sprite_;
  sprite_ = nullptr;

  delete particleEmitter_;
  particleEmitter_ = nullptr;

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    delete sprites_[i];
  }
  sprites_.clear();

  auto *sm = SoundManager::GetInstance();
  sm->StopBGM();
  sm->StopAllSE();

  // ゲームシーンだけで使う運用ならここで解放してOK
  sm->Unload("bgm_mokugyo");
  sm->Unload("se_fanfare");
}

void DebugScene::Update() {

  // ゲームシーンに移行
  if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
    SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
  }

  // テクスチャ差し替え
  if (engine_->GetInputManager()->IsTriggerKey(DIK_SPACE)) {
    sprites_[0]->ChangeTexture("resources/uvChecker.png");
  }

  // デバッグカメラ切り替え
  if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
    if (useDebugCamera_) {
      useDebugCamera_ = false;
    } else {
      useDebugCamera_ = true;
    }
  }

  auto *sm = SoundManager::GetInstance();

  // BGMを停止
  if (engine_->GetInputManager()->IsTriggerKey(DIK_B)) {
    sm->StopBGM();
  }

  // VキーでSE(重複可能）
  if (engine_->GetInputManager()->IsTriggerKey(DIK_V)) {
    SoundManager::GetInstance()->PlaySE("se_fanfare");
  }

  //=======================
  // スプライトの更新
  //=======================

  // スプライトの情報を呼び出す
  spritePosition_ = sprite_->GetPosition();
  spriteRotation_ = sprite_->GetRotation();
  spriteColor_ = sprite_->GetColor();
  spriteSize_ = sprite_->GetSize();
  spriteAnchorPoint_ = sprite_->GetAnchorPoint();

  // スプライトに設定
  sprite_->SetPosition(spritePosition_);
  sprite_->SetRotation(spriteRotation_);
  sprite_->SetColor(spriteColor_);
  sprite_->SetSize(spriteSize_);
  sprite_->SetAnchorPoint(spriteAnchorPoint_);
  sprite_->SetIsFlipX(isFlipX_);
  sprite_->SetIsFlipY(isFlipY_);

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    // spritePositions[i].x++;
    sprites_[i]->SetPosition(spritePositions_[i]);
    sprites_[i]->SetSize(spriteSizes_[i]);
  }

  sprite_->Update(uvTransformSprite_);

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    sprites_[i]->Update(uvTransformSprite_);
  }

  //=======================
  // 3Dオブジェクトの更新
  //=======================
  // rotateObj_ += 0.01f;

  object3d_->SetRotation({0.0f, rotateObj_, 0.0f});

  object3dA_->SetRotation({0.0f, rotateObj_, 0.0f});
  // object3dA_->SetTranslation({1.0f, 1.0f, 0.0f});

  object3d_->Update();
  object3dA_->Update();

  // エミッタ更新
  particleEmitter_->Update();

  //=======================
  // カメラの更新
  //=======================
  GameCamera *activeCamera = nullptr;

  camera_->SetRotate(cameraTransform_.rotate);
  camera_->SetTranslate(cameraTransform_.translate);

  // カメラの更新処理

  if (useDebugCamera_) {
    debugCamera_->Update(*engine_->GetInputManager());
    activeCamera = debugCamera_->GetCamera();
  } else {
    camera_->Update();
    activeCamera = camera_;
  }

  // アクティブカメラを描画で使用する
  engine_->GetObject3dRenderer()->SetDefaultCamera(activeCamera);

  // view / projection を作って ParticleManager 更新
  ParticleManager::GetInstance()->Update(activeCamera->GetViewMatrix(),
                                         activeCamera->GetProjectionMatrix());

#ifdef USE_IMGUI
  auto *renderer = engine_->GetObject3dRenderer();

  static bool showDirectionalLight = true;
  static bool showPointLight = false;
  static bool showSpotLight = false;
  static float directionalIntensityBackup = 1.0f;
  static float pointIntensityBackup = 1.0f;
  static float spotIntensityBackup = 4.0f;

  ImGui::Begin("Lighting");

  // DirectionalLight
  bool changedDirectional =
      ImGui::Checkbox("Enable DirectionalLight", &showDirectionalLight);
  if (changedDirectional) {
    if (auto *dl = renderer->GetDirectionalLightData()) {
      if (!showDirectionalLight) {
        directionalIntensityBackup = dl->intensity;
        dl->intensity = 0.0f;
      } else {
        dl->intensity = (directionalIntensityBackup > 0.0f)
                            ? directionalIntensityBackup
                            : 1.0f;
      }
    }
  }

  // PointLight
  bool changedPoint = ImGui::Checkbox("Enable PointLight", &showPointLight);
  if (changedPoint) {
    if (auto *pl = renderer->GetPointLightData()) {
      if (!showPointLight) {
        pointIntensityBackup = pl->intensity;
        pl->intensity = 0.0f;
      } else {
        pl->intensity =
            (pointIntensityBackup > 0.0f) ? pointIntensityBackup : 1.0f;
      }
    }
  }

  // SpotLight
  bool changedSpot = ImGui::Checkbox("Enable SpotLight", &showSpotLight);
  if (changedSpot) {
    if (auto *sl = renderer->GetSpotLightData()) {
      if (!showSpotLight) {
        spotIntensityBackup = sl->intensity;
        sl->intensity = 0.0f;
      } else {
        sl->intensity =
            (spotIntensityBackup > 0.0f) ? spotIntensityBackup : 1.0f;
      }
    }
  }

  ImGui::End();

  //========================
  // DirectionalLight
  //========================
  if (auto *dl = renderer->GetDirectionalLightData()) {
    if (showDirectionalLight) {
      ImGui::Begin("DirectionalLight");
      ImGui::ColorEdit3("Color", &dl->color.x);
      ImGui::DragFloat("Intensity", &dl->intensity, 0.01f, 0.0f, 10.0f);
      ImGui::End();
    }
  }

  //========================
  // PointLight
  //========================
  if (auto *pl = renderer->GetPointLightData()) {
    if (showPointLight) {
      ImGui::Begin("PointLight");

      ImGui::ColorEdit3("Color", &pl->color.x);
      ImGui::DragFloat3("Position", &pl->position.x, 0.05f, -20.0f, 20.0f);
      ImGui::DragFloat("Intensity", &pl->intensity, 0.05f, 0.0f, 10.0f);
      ImGui::DragFloat("Radius", &pl->radius, 0.1f, 0.01f, 100.0f);
      ImGui::DragFloat("Decay", &pl->decay, 0.05f, 0.01f, 8.0f);
      ImGui::End();
    }
  }

  //========================
  // SpotLight
  //========================
  if (auto *sl = renderer->GetSpotLightData()) {
    if (showSpotLight) {
      ImGui::Begin("SpotLight");

      ImGui::ColorEdit3("Color", &sl->color.x);
      ImGui::DragFloat3("Position", &sl->position.x, 0.05f, -20.0f, 20.0f);
      ImGui::DragFloat("Intensity", &sl->intensity, 0.05f, 0.0f, 10.0f);

      static float yawDeg = 0.0f;
      static float pitchDeg = -20.0f;
      ImGui::SliderFloat("Yaw(deg)", &yawDeg, -180.0f, 180.0f);
      ImGui::SliderFloat("Pitch(deg)", &pitchDeg, -89.0f, 89.0f);

      float yaw = DegToRad(yawDeg);
      float pitch = DegToRad(pitchDeg);
      sl->direction = {
          std::cos(pitch) * std::sin(yaw),
          std::sin(pitch),
          std::cos(pitch) * std::cos(yaw),
      };

      ImGui::DragFloat("Distance", &sl->distance, 0.1f, 0.01f, 100.0f);
      ImGui::DragFloat("Decay", &sl->decay, 0.05f, 0.01f, 8.0f);

      static float spotAngleDeg = 30.0f;
      ImGui::DragFloat("Angle(deg)", &spotAngleDeg, 0.1f, 1.0f, 89.0f);
      sl->cosAngle = std::cos(DegToRad(spotAngleDeg));
      ImGui::End();
    }
  }

#endif // USE_IMGUI
}

void DebugScene::Draw() {
  Draw3D();
  Draw2D();
}

void DebugScene::Draw3D() {
  engine_->Begin3D();

  // ここから下で3DオブジェクトのDrawを呼ぶ

  object3d_->Draw();
  object3dA_->Draw();
  // ParticleManager::GetInstance()->Draw();
}

void DebugScene::Draw2D() {
  engine_->Begin2D();

  // ここから下で2DオブジェクトのDrawを呼ぶ

  // for (uint32_t i = 0; i < kSpriteCount_; ++i) {
  // sprites_[i]->Draw();
  // }

  // sprite_->Draw();
}