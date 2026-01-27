#pragma once
#include "Math/Vector3.h"
#include <memory>

class Object3d;
class Object3dRenderer;

class Hazard {
public:
    Hazard() = default;
    ~Hazard();

    // 初期化: レンダラと位置・半径を与え視覚オブジェクトを生成
    void Initialize(Object3dRenderer* renderer, const Vector3& pos, float radius);
    // 更新: 内部オブジェクトを更新
    void Update(float dt);
    // 描画: 内部の Object3d を描画
    void Draw();

    Vector3 GetPosition() const { return position_; }
    float GetRadius() const { return radius_; }

private:
    Object3d* obj_ = nullptr;
    Vector3 position_{};
    float radius_ = 0.0f;
};
