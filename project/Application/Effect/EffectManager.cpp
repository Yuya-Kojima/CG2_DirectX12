#include "EffectManager.h"
#include "../../externals/nlohmann/json.hpp"
#include "Scene/SceneManager.h"
#include "Render/Renderer/PostProcess.h"
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
}

void EffectManager::Update(const Matrix4x4& viewProj) {
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
    // リセット処理
    if (postProcess->GetPostEffectType() == 10) {
      postProcess->SetPostEffectType(0);
      postProcess->SetShockwaves({});
    }
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
