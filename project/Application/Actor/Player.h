#pragma once
#include "Framework/BaseActor.h"
#include <memory>
#include <vector>

class LockOn;
class SpriteRenderer;
class ICamera;
class Object3d;

class Player : public BaseActor {
public:
    Player();
    ~Player() override;

    void Initialize() override;
    void Update() override;
    void Draw3D() override;
    void Draw2D() override;

    // シーンから情報を渡すためのセッター
    void SetSpriteRenderer(SpriteRenderer* renderer) { spriteRenderer_ = renderer; }
    void SetCamera(const ICamera* camera) { camera_ = camera; }
    
    // 今回はテストとして直接ターゲットを渡す
    void SetTarget(Object3d* target) { target_ = target; }

private:
    std::unique_ptr<LockOn> lockOn_;

    SpriteRenderer* spriteRenderer_ = nullptr;
    const ICamera* camera_ = nullptr;
    Object3d* target_ = nullptr;
};
