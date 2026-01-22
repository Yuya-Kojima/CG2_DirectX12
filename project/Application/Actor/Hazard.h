#pragma once
#include "Math/Vector3.h"
#include <memory>

class Object3d;
class Object3dRenderer;

class Hazard {
public:
    Hazard() = default;
    ~Hazard();

    void Initialize(Object3dRenderer* renderer, const Vector3& pos, float radius);
    void Update(float dt);
    void Draw();

    Vector3 GetPosition() const { return position_; }
    float GetRadius() const { return radius_; }

private:
    Object3d* obj_ = nullptr;
    Vector3 position_{};
    float radius_ = 0.0f;
};
