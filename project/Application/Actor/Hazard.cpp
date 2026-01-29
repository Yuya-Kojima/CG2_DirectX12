#include "Actor/Hazard.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
#include "Model/ModelManager.h"
#include <assert.h>

Hazard::~Hazard() {
    // Object3d は生ポインタで保持しているため破棄する
    if (obj_) { delete obj_; obj_ = nullptr; }
}

void Hazard::Initialize(Object3dRenderer* renderer, const Vector3& pos, float radius) {
    position_ = pos;
    radius_ = radius;
    if (renderer) {
        obj_ = new Object3d();
        obj_->Initialize(renderer);
        ModelManager::GetInstance()->LoadModel("breakableBlock.obj");
        obj_->SetModel("breakableBlock.obj");
        // モデルは直径に合わせてスケールを設定
        obj_->SetScale({radius*2.0f, radius*2.0f, radius*2.0f});
        obj_->SetTranslation(position_);
    }
}

void Hazard::Update(float dt) {
    if (obj_) obj_->Update();
}

void Hazard::Draw() {
    if (obj_) obj_->Draw();
}
