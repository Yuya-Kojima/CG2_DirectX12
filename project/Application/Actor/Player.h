#pragma once
#include "Math/Vector3.h"
#include "Camera/GameCamera.h"
#include <cstdint>

class Object3d;
class Object3dRenderer;
#include "Input/InputKeyState.h"

class Player {
public:
    Player();

    // Initialize with engine input manager and object3d renderer
    void Initialize(InputKeyState* input, Object3dRenderer* object3dRenderer);

    // Update and draw
    void Update(float deltaTime);
    void Draw();

    const Vector3& GetPosition() const { return position_; }
    void AttachCamera(GameCamera* camera) { camera_ = camera; }
    void SetPosition(const Vector3& pos);

private:
    // minimal: no Object3d creation here to avoid engine-side changes
    GameCamera* camera_ = nullptr;
    InputKeyState* input_ = nullptr;
    // 3D object for rendering
    Object3d* obj_ = nullptr;

    Vector3 position_{0.0f, 0.0f, 0.0f};
    Vector3 velocity_{0.0f, 0.0f, 0.0f};
    float rotate_ = 0.0f;

    float moveSpeed_ = 6.0f;
    float accel_ = 20.0f;
    float decel_ = 25.0f;
    float gravity_ = 30.0f;
    float jumpSpeed_ = 12.0f;
    bool onGround_ = true;
    float halfSize_ = 0.5f;
};
