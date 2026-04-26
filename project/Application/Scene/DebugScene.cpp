#include "DebugScene.h"
#include "Camera/GameCamera.h"
#include "Debug/ImGuiManager.h"
#include "Debug/Logger.h"
#include "Input/Input.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Particle/Particle.h"
#include "Particle/ParticleManager.h"
#include "Render/Primitive/Sphere.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/SceneManager.h"
#include "Texture/TextureManager.h"
#include <cmath>
#include <fstream>
#include <imgui.h>

void DebugScene::Initialize(EngineBase *engine) {

  engine_ = engine;

  //===========================
  // テクスチャファイルの読み込み
  //===========================

  // テクスチャの読み込み
  // TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

  // TextureManager::GetInstance()->LoadTexture("resources/monsterball.png");

  //===========================
  // オーディオファイルの読み込み
  //===========================

  auto *sm = SoundManager::GetInstance();

  // Audioファイルを登録
  sm->Load("mokugyo", "resources/mokugyo.wav");
  sm->Load("se", "resources/fanfare.wav");

  // bgm再生
  sm->PlayBGM("mokugyo");

  //===========================
  // スプライト関係の初期化
  //===========================

  // スプライトの生成と初期化
  for (int i = 0; i < kSpriteCount_; ++i) {
    auto sprite = std::make_unique<Sprite>();
    if (i % 2 == 1) {
      sprite->Initialize(engine_->GetSpriteRenderer(),
                         "resources/white1x1.png");
    } else {
      sprite->Initialize(engine_->GetSpriteRenderer(),
                         "resources/monsterBall.png");
    }
    sprites_.push_back(std::move(sprite));
  }

  sprite_ = std::make_unique<Sprite>();
  sprite_->Initialize(engine_->GetSpriteRenderer(), "resources/white1x1.png");

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
  camera_ = std::make_unique<GameCamera>();
  camera_->SetRotate({0.3f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 4.0f, -10.0f});
  engine_->GetObject3dRenderer()->SetDefaultCamera(camera_.get());

  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f},
      {0.3f, 0.0f, 0.0f},
      {0.0f, 4.0f, -10.0f},
  };

  // デバッグカメラ
  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize({0.0f, 4.0f, -10.0f});

  // モデルの読み込み
  ModelManager::GetInstance()->LoadModel("terrain.obj");

  ModelManager::GetInstance()->LoadModel("suzanne.obj");

  ModelManager::GetInstance()->LoadModel("AnimatedCube.gltf");

  // オブジェクトの生成と初期化
  object3d_ = std::make_unique<Object3d>();
  object3d_->Initialize(engine_->GetObject3dRenderer());
  object3d_->SetModel("terrain.obj");

  object3dA_ = std::make_unique<Object3d>();
  object3dA_->Initialize(engine_->GetObject3dRenderer());
  object3dA_->SetModel("suzanne.obj");
  object3dA_->SetEnvironmentCoefficient(1.0f);
  object3dA_->SetTranslation({3.0f, 1.0f, 0.0f});

  animatedCube_ = std::make_unique<Object3d>();
  animatedCube_->Initialize(engine_->GetObject3dRenderer());
  animatedCube_->SetModel("AnimatedCube.gltf");
  animation_ = LoadAnimationFile("resources/AnimatedCube", "AnimatedCube.gltf");
  animatedCube_->PlayAnimation(animation_);
  animatedCube_->SetTranslation({-3.0f, 1.0f, 0.0f});

  // ==========================
  // スケルトンのデバッグ描画用初期化
  // ==========================
  sneakWalk_ = std::make_unique<SkinnedObject>();
  sneakWalk_->Initialize(engine_->GetObject3dRenderer(),
                         engine_->GetSrvManager(), "resources/human",
                         "sneakWalk.gltf");

  Sphere sphere;
  sphere.Build(16, 0.02f);

  std::vector<Model::VertexData> sphereVertices;
  for (const auto &v : sphere.GetVertices()) {
    sphereVertices.push_back({v.position, v.texcoord, v.normal});
  }

  sphereModel_ = std::make_unique<Model>();
  sphereModel_->InitializeFromVertices(
      ModelManager::GetInstance()->GetModelRenderer(), sphereVertices);

  jointObjects_.clear();
  for (size_t i = 0; i < sneakWalk_->GetSkeleton().joints.size(); ++i) {
    auto jointObj = std::make_unique<Object3d>();
    jointObj->Initialize(engine_->GetObject3dRenderer());
    jointObj->SetModel(sphereModel_.get());
    jointObj->SetColor(
        Vector4{1.0f, 0.0f, 0.0f, 1.0f}); // 分かりやすいように赤色にする
    jointObjects_.push_back(std::move(jointObj));
  }

  //===========================
  // SkyBox
  //===========================
  TextureManager::GetInstance()->LoadTexture("resources/Skybox/Skybox.dds");

  skybox_ = std::make_unique<Skybox>();
  skybox_->Initialize(engine_->GetSkyboxRenderer());
  skybox_->SetTexture("resources/Skybox/Skybox.dds");
  skybox_->SetCamera(camera_.get());
  skybox_->SetScale({100.0f, 100.0f, 100.0f});

  //===========================
  // パーティクル関係の初期化
  //===========================

  // グループ（実体）の作成と初期化
  testParticleGroup_ = std::make_unique<BillboardParticleEmitter>();
  testParticleGroup_->Initialize("resources/circle.png");

  clearParticleGroup_ = std::make_unique<BillboardParticleEmitter>();
  clearParticleGroup_->Initialize("resources/uvChecker.png");

  hitParticleGroup_ = std::make_unique<MeshParticleEmitter>();
  hitParticleGroup_->InitializeAsRing("resources/white1x1.png", 64, 1.0f, 0.2f);

  cylinderParticleGroup_ = std::make_unique<MeshParticleEmitter>();
  cylinderParticleGroup_->InitializeAsCylinder("resources/gradationLine.png",
                                               32, 1.0f, 1.0f, 3.0f);

  planeHitParticleGroup_ = std::make_unique<MeshParticleEmitter>();
  planeHitParticleGroup_->InitializeAsPlane("resources/uvChecker.png");

  // 中心からXY方向にランダム速度
  particleEmitter_ = std::make_unique<ParticleEmitter>(
      testParticleGroup_.get(),  // 対象グループ
      Vector3{0.0f, 0.0f, 5.0f}, // center (エミッターの中心座標)
      Vector3{0.2f, 0.2f, 0.2f}, // halfSize（エミッターの半径）
      3,                         // 一度に発生させる数
      1.0f,                      // 発生間隔
      Vector3{0.0f, 0.0f, 0.0f}, // baseVel
      Vector3{1.0f, 1.0f, 1.0f}, // velRandom
      1.6f,                      // lifeMin
      2.1f                       // lifeMax
  );

  // 噴水のように下から吹き上げる
  clearParticleEmitter_ = std::make_unique<ParticleEmitter>(
      clearParticleGroup_.get(), Vector3{1.0f, 0.0f, 2.0f},
      Vector3{0.2f, 0.0f, 0.2f}, 8, 1.0f, Vector3{0.0f, 6.0f, 0.0f},
      Vector3{1.5f, 1.0f, 1.5f}, 0.6f, 1.1f);

  hitParticleEmitter_ = std::make_unique<ParticleEmitter>(
      hitParticleGroup_.get(), Vector3{0.0f, 2.0f, 0.0f}, Vector3{}, 8, 3.0f,
      Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 0.3f, 0.55f);

  hitParticleEmitter_->SetBaseScale(Vector3{0.05f, 1.0f, 1.0f});
  hitParticleEmitter_->SetRotateRandom(Vector3{0.0f, 0.0f, 2.0f});

  cylinderEmitter_ = std::make_unique<ParticleEmitter>(
      cylinderParticleGroup_.get(), Vector3{0.0f, 0.0f, 0.0f}, Vector3{}, 1,
      0.2f, Vector3{0, 0, 0}, Vector3{0, 0, 0}, 0.5f, 0.6f);

  planeHitParticleEmitter_ = std::make_unique<ParticleEmitter>(
      planeHitParticleGroup_.get(), Vector3{-2.0f, 2.0f, 0.0f}, Vector3{}, 3,
      1.5f, Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 0.5f, 1.0f);
  planeHitParticleEmitter_->SetBaseScale(Vector3{2.0f, 2.0f, 1.0f});
  planeHitParticleEmitter_->SetRotateRandom(Vector3{0.0f, 0.0f, 3.14f});
}

void DebugScene::Finalize() {
  testParticleGroup_->ClearParticles();
  clearParticleGroup_->ClearParticles();
  hitParticleGroup_->ClearParticles();
  cylinderParticleGroup_->ClearParticles();
  planeHitParticleGroup_->ClearParticles();

  auto *sm = SoundManager::GetInstance();
  sm->StopBGM();
  sm->StopAllSE();

  // ゲームシーンだけで使う運用ならここで解放してOK
  sm->Unload("mokugyo");
  sm->Unload("se");
}

void DebugScene::Update() {

  // input取得
  auto *input = engine_->GetInputManager();

  // Sound更新
  SoundManager::GetInstance()->Update();

  //==============================
  // ゲームシーンに移行
  //==============================

  // フェードはデフォで 0.35秒 & 黒色
  if (engine_->GetInputManager()->IsKeyTrigger(KeyCode::Enter)) {
    SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
  }

  if (input->IsPadTrigger(PadButton::A)) {
    SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
  }

  // フェードにかかる時間とフェードタイプと色とイージングを指定できる
  if (engine_->GetInputManager()->IsKeyTrigger(KeyCode::C)) {

    // 0.5秒
    // WipeLeft(左から右へ塗りつぶし)
    // 白色フェード
    // EaseOut
    //
    // ※引数がなかった場合デフォルト値が与えられる※
    SceneManager::GetInstance()->SetNextTransitionFade(
        0.5f, Fade::FadeType::WipeLeft, Vector4{1.0f, 1.0f, 1.0f, 1.0f});

    SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
  }

  // テクスチャ差し替え
  if (engine_->GetInputManager()->IsTriggerKey(DIK_SPACE)) {
    sprites_[0]->ChangeTexture("resources/uvChecker.png");

    sprites_[1]->SetColor({0.0f, 0.0f, 0.0f, 1.0f});
  }

  if (engine_->GetInputManager()->IsTriggerKey(DIK_SPACE)) {
    object3dA_->SetColor(Vector4{1.0f, 1.0f, 0.0f, 1.0f});
  }

  // デバッグカメラ切り替え

#ifdef USE_IMGUI

  if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
    if (useDebugCamera_) {
      useDebugCamera_ = false;
    } else {
      useDebugCamera_ = true;
    }
  }

#endif // USE_IMGUI

  auto *sm = SoundManager::GetInstance();

  // BGMを停止
  if (engine_->GetInputManager()->IsTriggerKey(DIK_B)) {
    sm->StopBGM();
  }

  // VキーでSE(重複可能）
  if (engine_->GetInputManager()->IsTriggerKey(DIK_V)) {
    SoundManager::GetInstance()->PlaySE("se");
  }

  // Bキーで重複不可のSE
  if (engine_->GetInputManager()->IsKeyTrigger(KeyCode::B)) {
    SoundManager::GetInstance()->PlaySE_Once("se");
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

  sprites_[0]->SetScale(Vector2{1.5f, 1.5f});

  sprite_->Update(uvTransformSprite_);

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    sprites_[i]->Update(uvTransformSprite_);
  }

  //=======================
  // 3Dオブジェクトの更新
  //=======================

  object3d_->Update();

  // rotateObj_ += 0.01f;
  // object3dA_->SetRotation({0.0f, rotateObj_, 0.0f});
  object3dA_->Update();

  animatedCube_->Update();
  // スケルトンのアニメーション更新
  sneakWalk_->Update(1.0f / 60.0f);

  for (size_t i = 0; i < sneakWalk_->GetSkeleton().joints.size(); ++i) {
    const Joint &joint = sneakWalk_->GetSkeleton().joints[i];
    Vector3 pos = {joint.skeletonSpaceMatrix.m[3][0],
                   joint.skeletonSpaceMatrix.m[3][1],
                   joint.skeletonSpaceMatrix.m[3][2]};
    jointObjects_[i]->SetTranslation(pos);
    jointObjects_[i]->Update();
  }

  //=======================
  // パーティクルの更新
  //=======================

  /* エミッタの更新(パーティクルを生成間隔に基づいて自動生成)
  --------------------------------------------------*/
  particleEmitter_->Update();
  clearParticleEmitter_->Update();
  hitParticleEmitter_->Update();
  cylinderEmitter_->Update();
  planeHitParticleEmitter_->Update();

  /* 手動でパーティクルを生成することも可能
  -------------------------------------*/
  if (input->IsKeyTrigger(KeyCode::E)) {

    // デフォルトカラーで生成
    clearParticleEmitter_->Emit();

  } else if (input->IsKeyTrigger(KeyCode::F)) {

    // 色を指定して生成
    clearParticleEmitter_->Emit(Vector4{0.0f, 0.0f, 1.0f, 1.0f});
  }

  //=======================
  // カメラの更新
  //=======================
  const ICamera *activeCamera = nullptr;

  camera_->SetRotate(cameraTransform_.rotate);
  camera_->SetTranslate(cameraTransform_.translate);

  // カメラの更新処理

  if (useDebugCamera_) {
    debugCamera_->Update(*engine_->GetInputManager());
    activeCamera = debugCamera_->GetCamera();
  } else {
    camera_->Update();
    activeCamera = camera_.get();
  }

  // アクティブカメラを描画で使用する
  engine_->GetObject3dRenderer()->SetDefaultCamera(activeCamera);

  // 個別のパーティクルグループを更新
  testParticleGroup_->Update(activeCamera->GetViewMatrix(),
                             activeCamera->GetProjectionMatrix());
  clearParticleGroup_->Update(activeCamera->GetViewMatrix(),
                              activeCamera->GetProjectionMatrix());
  hitParticleGroup_->Update(activeCamera->GetViewMatrix(),
                            activeCamera->GetProjectionMatrix());
  cylinderParticleGroup_->Update(activeCamera->GetViewMatrix(),
                                 activeCamera->GetProjectionMatrix());
  planeHitParticleGroup_->Update(activeCamera->GetViewMatrix(),
                                 activeCamera->GetProjectionMatrix());

  // 天球の更新
  if (skybox_) {
    skybox_->SetCamera(activeCamera);
    skybox_->Update();
  }

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

  if (skybox_) {
    engine_->GetSkyboxRenderer()->Begin();
    skybox_->Draw();

    // Skybox描画後に通常3Dへ戻す
    engine_->GetObject3dRenderer()->Begin();
  }

  object3d_->Draw();
  object3dA_->Draw();
  animatedCube_->Draw();

  // CSで計算済みの頂点を使って描画されるため、通常のBegin()のままでよい
  sneakWalk_->Draw();

  // スケルトンのデバッグ描画（最前面に表示するためZテスト無効）
  engine_->GetObject3dRenderer()->SetDepthEnable(false);

  // 1. 関節間のLineを登録
  for (size_t i = 0; i < sneakWalk_->GetSkeleton().joints.size(); ++i) {
    const Joint &joint = sneakWalk_->GetSkeleton().joints[i];
    if (joint.parent) {
      const Joint &parentJoint =
          sneakWalk_->GetSkeleton().joints[*joint.parent];
      Vector3 start = {parentJoint.skeletonSpaceMatrix.m[3][0],
                       parentJoint.skeletonSpaceMatrix.m[3][1],
                       parentJoint.skeletonSpaceMatrix.m[3][2]};
      Vector3 end = {joint.skeletonSpaceMatrix.m[3][0],
                     joint.skeletonSpaceMatrix.m[3][1],
                     joint.skeletonSpaceMatrix.m[3][2]};
      engine_->GetLineRenderer()->DrawLine(start, end,
                                           {1.0f, 1.0f, 1.0f, 1.0f}); // 白い線
    }
  }

  // 3. 関節のSphereを描画
  for (auto &jointObj : jointObjects_) {
    jointObj->Draw();
  }
  engine_->GetObject3dRenderer()->SetDepthEnable(true); // 元に戻す

  testParticleGroup_->Draw();
  // clearParticleGroup_->Draw();
  hitParticleGroup_->Draw();
  cylinderParticleGroup_->Draw();
  planeHitParticleGroup_->Draw();

  // 2. Lineを描画 (他の描画のパイプラインを壊さないように最後に呼ぶ)
  if (const ICamera *camera =
          engine_->GetObject3dRenderer()->GetDefaultCamera()) {
    engine_->GetLineRenderer()->Render(camera->GetViewProjectionMatrix());
  }
}

void DebugScene::Draw2D() {
  engine_->Begin2D();

  // ここから下で2DオブジェクトのDrawを呼ぶ

  for (uint32_t i = 0; i < kSpriteCount_; ++i) {
    sprites_[i]->Draw();
  }

  // sprite_->Draw();
}

DebugScene::~DebugScene() = default;
