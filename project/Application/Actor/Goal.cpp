#include "Actor/Goal.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
#include "Model/ModelManager.h"

void Goal::Initialize(Object3dRenderer* renderer, const Vector3& pos) {
    // レンダラ参照と位置を保存し、描画用オブジェクトを生成
    renderer_ = renderer; pos_ = pos;
    obj_ = std::make_unique<Object3d>();
    obj_->Initialize(renderer_);
    std::string m = ModelManager::ResolveModelPath("goal.obj");
    ModelManager::GetInstance()->LoadModel(m);
    obj_->SetModel(m);
    obj_->SetScale({0.8f,0.8f,0.8f});
    obj_->SetTranslation(pos_);
}

void Goal::Update(float dt) {
    // カメラが存在する場合のみ Object3d を更新（視界外などでは不要な更新を避ける）
    auto* cam = renderer_->GetDefaultCamera();
    if (cam) obj_->Update();
}

void Goal::Draw() { if (obj_) obj_->Draw(); }
