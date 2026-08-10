#include "EffectManager.h"
#include "../../externals/nlohmann/json.hpp"
#include "Scene/SceneManager.h"
#include "Render/Renderer/PostProcess.h"
#include "Camera/ICamera.h"
#include "Camera/RailCamera.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif
#include <fstream>
#include <filesystem>
#include <iomanip>

EffectManager* EffectManager::GetInstance() {
  static EffectManager instance;
  return &instance;
}

void EffectManager::Initialize() {
  LoadShockwaveConfig();

  bossTelegraphParticleGroup_ = std::make_unique<BillboardParticleEmitter>();
  bossTelegraphParticleGroup_->Initialize("resources/gradationLine.png"); // グラデーションラインを使ってシャープなリングに
  bossTelegraphParticleGroup_->SetIsRingMode(true); // 3Dの輪っかが収束するSF的な演出にする

  bossTelegraphEmitter_ = std::make_unique<ParticleEmitter>(
      bossTelegraphParticleGroup_.get(), Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 1, 0.0f,
      Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 0.4f, 0.4f); // 寿命を少し伸ばして輪が迫ってくる過程を見せる
  bossTelegraphEmitter_->SetBaseScale({50.0f, 50.0f, 50.0f}); // 大きな輪
  bossTelegraphEmitter_->SetScaleRandom({20.0f, 20.0f, 20.0f});
  bossTelegraphEmitter_->SetColor({3.0f, 1.2f, 0.1f, 0.15f}); 
  bossTelegraphEmitter_->SetScaleVelocity({-200.0f, -200.0f, -200.0f}); 
  // ランダムな角度（X, Y, Z軸）からリングが発生するようにして、球状にエネルギーが集まるように見せる
  bossTelegraphEmitter_->SetRotateRandom({3.14f, 3.14f, 3.14f});

  bossBurstParticleGroup_ = std::make_unique<BillboardParticleEmitter>();
  bossBurstParticleGroup_->Initialize("resources/circle.png");
  bossBurstParticleGroup_->SetIsRingMode(false);

  bossBurstEmitter_ = std::make_unique<ParticleEmitter>(
      bossBurstParticleGroup_.get(), Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 1, 0.0f,
      Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}, 0.3f, 0.3f); // 発散エフェクトの寿命
  bossBurstEmitter_->SetBaseScale({10.0f, 10.0f, 10.0f}); // 最初は小さめ
  bossBurstEmitter_->SetColor({4.0f, 3.0f, 1.0f, 0.8f}); // 強い白・黄色系でフラッシュ
  bossBurstEmitter_->SetScaleVelocity({400.0f, 400.0f, 400.0f}); // 超高速で膨張する（発散）

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
}

void EffectManager::Update(const ICamera* camera) {
  if (!camera) return;
  Matrix4x4 viewProj = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());

  auto postProcess = SceneManager::GetInstance()->GetCurrentScenePostProcess();
  if (!postProcess) return;

  if (!activeShockwaves_.empty()) {
    // タイマー更新
    for (auto it = activeShockwaves_.begin(); it != activeShockwaves_.end(); ) {
      it->timer -= 1.0f / 60.0f;
      if (it->timer <= 0.0f) {
        it = activeShockwaves_.erase(it);
      } else {
        ++it;
      }
    }
  }

  if (!activeShockwaves_.empty()) {
    postProcess->SetPostEffectType(10); // 10: Shockwave

    std::vector<PostProcess::ShockwaveParams> shockwaveParams;
    for (const auto& sw : activeShockwaves_) {
      Vector3 pos = sw.worldPos;
      float w = pos.x * viewProj.m[0][3] + pos.y * viewProj.m[1][3] + pos.z * viewProj.m[2][3] + viewProj.m[3][3];
      if (w <= 0.0f) w = 0.0001f;

      Vector3 ndcPos = {
          (pos.x * viewProj.m[0][0] + pos.y * viewProj.m[1][0] + pos.z * viewProj.m[2][0] + viewProj.m[3][0]) / w,
          (pos.x * viewProj.m[0][1] + pos.y * viewProj.m[1][1] + pos.z * viewProj.m[2][1] + viewProj.m[3][1]) / w,
          (pos.x * viewProj.m[0][2] + pos.y * viewProj.m[1][2] + pos.z * viewProj.m[2][2] + viewProj.m[3][2]) / w
      };
      
      float uvX = (ndcPos.x + 1.0f) * 0.5f;
      float uvY = (1.0f - ndcPos.y) * 0.5f;
      float t = sw.timer / shockwaveConfig_.duration;
      
      PostProcess::ShockwaveParams param;
      param.center[0] = uvX;
      param.center[1] = uvY;
      param.radius = (1.0f - t) * shockwaveConfig_.maxRadius;
      param.thickness = shockwaveConfig_.thickness;
      param.weight = t;
      param.distortion = shockwaveConfig_.distortion;
      
      shockwaveParams.push_back(param);
    }
    postProcess->SetShockwaves(shockwaveParams);
  } else {
    // リセット
    if (postProcess && postProcess->GetPostEffectType() == 10) {
      postProcess->SetPostEffectType(0);
      postProcess->SetShockwaves({});
    }
  }

  // パーティクルの更新
  for (int i = 0; i < kMaxHitEffects; ++i) {
      deathCoreEmitters_[i]->Update();
      deathFlareEmitters_[i]->Update();
      deathRingEmitters_[i]->Update();
      hitCoreParticleGroups_[i]->Update(camera->GetViewMatrix(), camera->GetProjectionMatrix());
      hitFlareParticleGroups_[i]->Update(camera->GetViewMatrix(), camera->GetProjectionMatrix());
      hitRingParticleGroups_[i]->Update(camera->GetViewMatrix(), camera->GetProjectionMatrix());
  }

  if (bossTelegraphEmitter_) {
      bossTelegraphEmitter_->Update();
      bossTelegraphParticleGroup_->Update(camera->GetViewMatrix(), camera->GetProjectionMatrix());
  }

  if (bossBurstEmitter_) {
      bossBurstEmitter_->Update();
      bossBurstParticleGroup_->Update(camera->GetViewMatrix(), camera->GetProjectionMatrix());
  }
}

void EffectManager::Draw() {
  for (int i = 0; i < kMaxHitEffects; ++i) {
      hitCoreParticleGroups_[i]->Draw();
      hitFlareParticleGroups_[i]->Draw();
      hitRingParticleGroups_[i]->Draw();
  }

  if (bossTelegraphParticleGroup_) {
      bossTelegraphParticleGroup_->Draw();
  }

  if (bossBurstParticleGroup_) {
      bossBurstParticleGroup_->Draw();
  }
}

void EffectManager::PlayBossTelegraphEffect(const Vector3& center, float chargeRatio) {
  if (bossTelegraphEmitter_) {
    // chargeRatio (0.0 ~ 1.0) に応じてエフェクトを激しくする
    
    // 最初はゆっくり迫る大きな輪、最後は超高速で吸い込まれる中くらいの輪
    float currentSize = 80.0f - (30.0f * chargeRatio); 
    bossTelegraphEmitter_->SetBaseScale({currentSize, currentSize, currentSize});
    
    // 吸い込みスピードも徐々に速くする (-150.0f -> -400.0f)
    float currentSpeed = -150.0f - (250.0f * chargeRatio);
    bossTelegraphEmitter_->SetScaleVelocity({currentSpeed, currentSpeed, currentSpeed});
    
    bossTelegraphEmitter_->SetCenter(center);
    
    // 最初は少なめ、最後は大量のリングが球状に押し寄せる
    float emitChance = 0.3f + (0.7f * chargeRatio); 
    if ((rand() % 100) / 100.0f <= emitChance) {
        bossTelegraphEmitter_->Emit();
    }
  }
}

void EffectManager::PlayBossBurstEffect(const Vector3& center) {
  if (bossBurstEmitter_) {
    bossBurstEmitter_->SetCenter(center);
    bossBurstEmitter_->Emit();
  }
  // 画面の歪み（ショックウェーブ）も同時に発生させて衝撃を表現
  PlayShockwave(center);
}

void EffectManager::PlayEnemyDeathEffect(const Vector3 &worldPos, const Vector4 &baseColor) {
  PlayShockwave(worldPos);

  int i = nextHitEffectIndex_;
  
  // 色を動的に変更してEmit
  deathCoreEmitters_[i]->SetCenter(worldPos);
  deathCoreEmitters_[i]->Emit(baseColor);
  
  // フレアは元の色にEnemyの色を少し混ぜるか、明るめにする
  Vector4 flareColor = {baseColor.x * 2.0f, baseColor.y * 2.0f, baseColor.z * 2.0f, 1.0f};
  deathFlareEmitters_[i]->SetCenter(worldPos);
  deathFlareEmitters_[i]->Emit(flareColor);
  
  // リングも同色系統にする
  Vector4 ringColor = {baseColor.x * 1.5f, baseColor.y * 1.5f, baseColor.z * 1.5f, 1.0f};
  deathRingEmitters_[i]->SetBaseScale({0.1f, 0.1f, 0.1f}); // 爆発用の初期サイズ
  deathRingEmitters_[i]->SetScaleVelocity({80.0f, 80.0f, 80.0f}); // 爆発用に高速で広がる
  deathRingEmitters_[i]->SetCenter(worldPos);
  deathRingEmitters_[i]->Emit(ringColor);

  nextHitEffectIndex_ = (nextHitEffectIndex_ + 1) % kMaxHitEffects;
}

void EffectManager::PlayEnemyDeathSimpleEffect(const Vector3 &worldPos, const Vector4 &baseColor) {
  int i = nextHitEffectIndex_;
  
  // コア（しっかり見えるようにサイズを30に拡大）
  deathCoreEmitters_[i]->SetBaseScale({30.0f, 30.0f, 30.0f});
  deathCoreEmitters_[i]->SetCenter(worldPos);
  deathCoreEmitters_[i]->Emit(baseColor);
  
  // フレア（敵のモデルより大きくなるようにサイズを2.5に拡大）
  Vector4 flareColor = {baseColor.x * 2.0f, baseColor.y * 2.0f, baseColor.z * 2.0f, 1.0f};
  deathFlareEmitters_[i]->SetBaseScale({2.5f, 2.5f, 2.5f});
  deathFlareEmitters_[i]->SetCenter(worldPos);
  deathFlareEmitters_[i]->Emit(flareColor);
  
  nextHitEffectIndex_ = (nextHitEffectIndex_ + 1) % kMaxHitEffects;
}

void EffectManager::PlayFunnelMuzzleRing(const Vector3 &worldPos, const Vector4 &color) {
  // 3枚のリングを少しずつサイズと速度をズラして重ねる
  for (int j = 0; j < 3; ++j) {
    int i = nextHitEffectIndex_;
    
    // リングごとに初期サイズと広がるスピードに差をつける
    float baseScale = 0.2f + (j * 0.15f);
    float speed = 15.0f - (j * 2.0f);
    
    // 発光を強くするためRGB成分を強調
    Vector4 brightColor = {color.x * 2.5f, color.y * 2.5f, color.z * 2.5f, color.w};

    deathRingEmitters_[i]->SetBaseScale({baseScale, baseScale, baseScale});
    deathRingEmitters_[i]->SetScaleVelocity({speed, speed, speed});
    deathRingEmitters_[i]->SetCenter(worldPos);
    deathRingEmitters_[i]->Emit(brightColor);

    nextHitEffectIndex_ = (nextHitEffectIndex_ + 1) % kMaxHitEffects;
  }
}

void EffectManager::PlayShockwave(const Vector3& worldPos) {
  if (activeShockwaves_.size() < 5) {
    activeShockwaves_.push_back({shockwaveConfig_.duration, worldPos});
  }
}

void EffectManager::DrawEditorUI(RailCamera* railCamera) {
#ifdef USE_IMGUI
  ImGui::Text("Effect Master Settings");
  ImGui::Separator();
  
  if (isShockwaveConfigDirty_) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.0f, 1.0f)); 
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.7f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
  }
  
  std::string buttonText = isShockwaveConfigDirty_ ? (const char*)u8"[* 未保存] Save Config" : (const char*)u8"Save Config";
  if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
    SaveShockwaveConfig();
  }
  
  if (isShockwaveConfigDirty_) {
    ImGui::PopStyleColor(3);
  }
  
  ImGui::Spacing();
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
  if (ImGui::Button((const char*)u8"▶ Test Play (テスト再生)", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
    if (railCamera) {
      Matrix4x4 viewMatrix = railCamera->GetViewMatrix();
      Matrix4x4 cameraWorld = Inverse(viewMatrix);
      Vector3 cameraPos = {cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2]};
      Vector3 cameraForward = {cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2]};
      Vector3 testPos = {
        cameraPos.x + cameraForward.x * 20.0f,
        cameraPos.y + cameraForward.y * 20.0f,
        cameraPos.z + cameraForward.z * 20.0f
      };
      PlayShockwave(testPos);
    }
  }
  ImGui::PopStyleColor();
  
  ImGui::Spacing();
  
  bool changed = false;
  changed |= ImGui::DragFloat((const char*)u8"再生時間 (Duration)", &shockwaveConfig_.duration, 0.01f, 0.1f, 5.0f);
  changed |= ImGui::DragFloat((const char*)u8"最大半径 (Max Radius)", &shockwaveConfig_.maxRadius, 0.01f, 0.1f, 5.0f);
  changed |= ImGui::DragFloat((const char*)u8"歪みの強さ (Distortion)", &shockwaveConfig_.distortion, 0.001f, 0.0f, 0.5f);
  changed |= ImGui::DragFloat((const char*)u8"波の太さ (Thickness)", &shockwaveConfig_.thickness, 0.001f, 0.0f, 1.0f);
  
  if (changed) {
    isShockwaveConfigDirty_ = true;
  }
#endif
}

void EffectManager::SaveShockwaveConfig() {
  nlohmann::json root;
  root["duration"] = shockwaveConfig_.duration;
  root["maxRadius"] = shockwaveConfig_.maxRadius;
  root["distortion"] = shockwaveConfig_.distortion;
  root["thickness"] = shockwaveConfig_.thickness;

  if (!std::filesystem::exists("resources/config")) {
    std::filesystem::create_directories("resources/config");
  }

  std::ofstream file("resources/config/ShockwaveConfig.json");
  if (file.is_open()) {
    file << std::setw(4) << root << std::endl;
    isShockwaveConfigDirty_ = false;
  }
}

void EffectManager::LoadShockwaveConfig() {
  std::ifstream file("resources/config/ShockwaveConfig.json");
  if (file.is_open()) {
    nlohmann::json root;
    try {
      file >> root;
      if (root.contains("duration")) shockwaveConfig_.duration = root["duration"];
      if (root.contains("maxRadius")) shockwaveConfig_.maxRadius = root["maxRadius"];
      if (root.contains("distortion")) shockwaveConfig_.distortion = root["distortion"];
      if (root.contains("thickness")) shockwaveConfig_.thickness = root["thickness"];
    } catch (...) {
      // Parse error, keep defaults
    }
  }
  isShockwaveConfigDirty_ = false;
}
