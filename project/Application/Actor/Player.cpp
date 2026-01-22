#include "Player.h"
#include "Input/InputKeyState.h"
#include "Camera/GameCamera.h"
#include "Math/MathUtil.h"
#include "Object3d/Object3d.h"
#include "Model/ModelManager.h"
#include <algorithm>
#include <filesystem>
#include "Debug/Logger.h"
#include <format>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

Player::Player() {}

void Player::Initialize(InputKeyState* input, Object3dRenderer* object3dRenderer) {
    input_ = input;
    position_ = {0.0f, 0.0f, 0.0f};

    // create Object3d and initialize with renderer
    if (object3dRenderer) {
        obj_ = new Object3d();
        obj_->Initialize(object3dRenderer);
        // try to assign a player model from a few candidate names
        std::vector<std::string> candidates = {"Player", "player.obj", "models/player.obj", "player"};
        bool assigned = false;
        for (const auto &c : candidates) {
            std::string resolved = ModelManager::ResolveModelPath(c);
            if (resolved != c || std::filesystem::exists(resolved)) {
                try {
                    Logger::Log(std::format("Player: candidate='{}' resolved='{}'\n", c, resolved));
                } catch (...) {}
                // ensure model is loaded
                ModelManager::GetInstance()->LoadModel(resolved);
                obj_->SetModel(resolved);
                assigned = true;
                try {
                    Logger::Log(std::format("Player: assigned model '{}'.\n", resolved));
                } catch (...) {}
                break;
            }
        }
        // no model assigned from candidates -> leave as-is (no forced fallback)
        // Set initial transform to make model visible (similar to DebugScene)
        obj_->SetTranslation(position_);
        obj_->SetScale({1.0f, 1.0f, 1.0f});
        obj_->SetRotation({0.0f, rotate_, 0.0f});

        // leave model assignment as resolved above; do not force fallback here
    }
}

void Player::Update(float dt) {
    if (!input_) return;

    float ax = 0.0f, az = 0.0f;
    if (input_->IsPressKey(DIK_A)) ax -= 1.0f;
    if (input_->IsPressKey(DIK_D)) ax += 1.0f;
    if (input_->IsPressKey(DIK_W)) az += 1.0f;
    if (input_->IsPressKey(DIK_S)) az -= 1.0f;

    float targetVX = ax * moveSpeed_;
    float targetVZ = az * moveSpeed_;

    if (velocity_.x < targetVX) {
        velocity_.x = std::min(targetVX, velocity_.x + accel_ * dt);
    } else if (velocity_.x > targetVX) {
        velocity_.x = std::max(targetVX, velocity_.x - decel_ * dt);
    }
    if (velocity_.z < targetVZ) {
        velocity_.z = std::min(targetVZ, velocity_.z + accel_ * dt);
    } else if (velocity_.z > targetVZ) {
        velocity_.z = std::max(targetVZ, velocity_.z - decel_ * dt);
    }

    if (onGround_ && input_->IsTriggerKey(DIK_SPACE)) {
        velocity_.y = jumpSpeed_;
        onGround_ = false;
    }

    // gravity
    velocity_.y -= gravity_ * dt;

    // integrate
    position_.x += velocity_.x * dt;
    position_.y += velocity_.y * dt;
    position_.z += velocity_.z * dt;

    // simple ground snap at y=0
    if (position_.y <= 0.0f) {
        position_.y = 0.0f;
        velocity_.y = 0.0f;
        onGround_ = true;
    }

    // camera follow (smooth)
    if (camera_) {
        Vector3 desired { position_.x - 6.0f, position_.y + 8.0f, position_.z - 6.0f };
        Vector3 current = camera_->GetTranslate();
        const float smooth = 10.0f;
        float t = std::min(1.0f, smooth * dt);
        Vector3 camPos;
        camPos.x = current.x + (desired.x - current.x) * t;
        camPos.y = current.y + (desired.y - current.y) * t;
        camPos.z = current.z + (desired.z - current.z) * t;
        camera_->SetTranslate(camPos);
        camera_->Update();
    }

    // update object transform
    if (obj_) {
        obj_->SetTranslation(position_);
        obj_->Update();
    }

    // (debug logging removed)
}

void Player::Draw() {
    if (obj_) {
        obj_->Draw();
    }
}

void Player::SetPosition(const Vector3& pos) {
    position_ = pos;
    if (obj_) {
        obj_->SetTranslation(position_);
    }
}
