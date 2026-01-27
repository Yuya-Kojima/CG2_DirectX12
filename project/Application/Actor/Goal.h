#pragma once
#include "Math/Vector3.h"
#include <memory>

class Object3d;
class Object3dRenderer;

class Goal {
public:
    // 初期化: レンダラと位置を与えて視覚オブジェクトを生成します
    void Initialize(Object3dRenderer* renderer, const Vector3& pos);
    // 更新: 必要に応じてオブジェクトの状態を更新します
    void Update(float dt);
    // 描画: 内部の Object3d を描画します
    void Draw();
    const Vector3& GetPosition() const { return pos_; }
private:
    Object3dRenderer* renderer_ = nullptr;
    std::unique_ptr<Object3d> obj_;
    Vector3 pos_{0,0,0};
};
