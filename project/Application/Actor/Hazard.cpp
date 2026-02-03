#include "Actor/Hazard.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
#include "Model/ModelManager.h"
#include "Model/Model.h"
#include "Debug/Logger.h"
#include <Windows.h>
#include <sstream>
#include <assert.h>
#include <filesystem>
#include <vector>

Hazard::~Hazard() {
    // Object3d は生ポインタで保持しているため破棄する
    if (obj_) { delete obj_; obj_ = nullptr; }
}

void Hazard::Initialize(Object3dRenderer* renderer, const Vector3& pos, float radius, const std::string& model) {
    position_ = pos;
    radius_ = radius;
    if (renderer) {
        obj_ = new Object3d();
        obj_->Initialize(renderer);
        // モデルアセットの検索とロード
        namespace fs = std::filesystem;
        const std::vector<std::string> candidates = { "needle.obj", "neadle.obj", "hazard.obj", "spike.obj", "Block.obj" };
        std::string chosen = "Block.obj";
        if (!model.empty()) {
            // If a model was explicitly provided, try it first
            chosen = model;
        } else {
            for (const auto &c : candidates) {
                std::string resolved = ModelManager::ResolveModelPath(c);
                // 検索して見つかった最初のモデルを使用
                if (resolved.find('/') != std::string::npos || resolved.find('\\') != std::string::npos) {
                    if (fs::exists(resolved)) {
                        chosen = resolved;
                        break;
                    }
                } else {
                    //  Check common resource paths
                    if (fs::exists(std::string("Application/resources/") + resolved)) { chosen = std::string("Application/resources/") + resolved; break; }
                    if (fs::exists(std::string("resources/") + resolved)) { chosen = std::string("resources/") + resolved; break; }
                }
            }
        }
        // Load and set the chosen model
        std::string resolved = ModelManager::ResolveModelPath(chosen);
        try {
            std::ostringstream oss;
            oss << "[Hazard] Initialize: pos=(" << position_.x << "," << position_.y << "," << position_.z << ") radius=" << radius_ << " model='" << chosen << "' -> resolved='" << resolved << "'\n";
            std::string msg = oss.str();
            Logger::Log(msg);
            OutputDebugStringA(msg.c_str());
        } catch(...) {}
        ModelManager::GetInstance()->LoadModel(resolved);
        obj_->SetModel(resolved);
        // Log model root local matrix and object transform to help diagnose origin/offset issues
        try {
            auto m = obj_->GetModel();
            std::ostringstream oss2;
            if (m) {
                const auto &mat = m->GetRootLocalMatrix();
                oss2 << "[Hazard] Model root local matrix:\n";
                for (int r = 0; r < 4; ++r) {
                    oss2 << "  ";
                    for (int c = 0; c < 4; ++c) {
                        oss2 << mat.m[r][c] << (c < 3 ? "," : "");
                    }
                    oss2 << "\n";
                }
            } else {
                oss2 << "[Hazard] Model pointer null after SetModel\n";
            }
            // also log transform we set
            oss2 << "[Hazard] Obj transform translate=(" << position_.x << "," << position_.y << "," << position_.z << ") scale=(" << radius_*2.0f << "," << radius_*2.0f << "," << radius_*2.0f << ")\n";
            std::string mmsg = oss2.str();
            Logger::Log(mmsg);
            OutputDebugStringA(mmsg.c_str());
        } catch(...) {}
        // モデルは直径に合わせてスケールを設定
        obj_->SetScale({radius*2.0f, radius*2.0f, radius*2.0f});

        // If the provided position had a Y of 0 (default ground), try to snap to level floor
        // so hazards sit flush with the floor visuals. Use Level::RaycastDown semantics
        // by probing slightly above and asking Level (if available) — but Level isn't
        // directly accessible here. We keep the position as given, but log a hint
        // if Y==0 so callers can place hazards at floor height.
        obj_->SetTranslation(position_);
    }
}

void Hazard::Update(float dt) {
    if (obj_) obj_->Update();
}

void Hazard::Draw() {
    if (obj_) obj_->Draw();
}
