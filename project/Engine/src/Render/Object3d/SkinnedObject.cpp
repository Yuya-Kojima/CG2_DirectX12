#include "Render/Object3d/SkinnedObject.h"
#include "Render/Model/ModelManager.h"
#include "Render/Renderer/Object3dRenderer.h"
#include <cmath>

void SkinnedObject::Initialize(Object3dRenderer* object3dRenderer, SrvManager* srvManager, const std::string& directoryPath, const std::string& filename) {
    // モデルデータの読み込み
    ModelManager::GetInstance()->LoadModel(directoryPath + "/" + filename);

    model_ = std::make_unique<Model>();
    model_->Initialize(ModelManager::GetInstance()->GetModelRenderer(), directoryPath, filename);

    // アニメーションの読み込み
    animation_ = LoadAnimationFile(directoryPath, filename);

    // スケルトンの作成
    skeleton_ = CreateSkeleton(model_->GetRootNode());

    // スキンクラスターの作成
    skinCluster_ = CreateSkinCluster(object3dRenderer->GetDx12Core(), srvManager, skeleton_, model_->GetModelData());

    // 描画用Object3dの作成と割り当て
    object3d_ = std::make_unique<Object3d>();
    object3d_->Initialize(object3dRenderer);
    object3d_->SetModel(model_.get());
    object3d_->SetSkinCluster(&skinCluster_);
}

void SkinnedObject::Update(float deltaTime) {
    // 1. アニメーション時間の更新
    animationTime_ += deltaTime;
    animationTime_ = std::fmod(animationTime_, animation_.duration);

    // 2. アニメーションの適用（各関節のローカル行列更新）
    ApplyAnimation(skeleton_, animation_, animationTime_);

    // 3. スケルトンの更新（親から子への行列伝播）
    ::Update(skeleton_);

    // 4. スキンクラスターの更新（パレットの更新）
    ::Update(skinCluster_, skeleton_);

    // 5. 3Dオブジェクトの全体更新（WVP行列などの計算）
    object3d_->Update();
}

void SkinnedObject::Draw() {
    if (object3d_) {
        object3d_->Draw();
    }
}
