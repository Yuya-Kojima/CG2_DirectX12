#include "Render/Object3d/SkinnedObject.h"
#include "Core/SrvManager.h"
#include "Render/Model/ModelManager.h"
#include "Render/Renderer/Object3dRenderer.h"
#include <cmath>

void SkinnedObject::Initialize(Object3dRenderer *object3dRenderer,
                               SrvManager *srvManager,
                               const std::string &directoryPath,
                               const std::string &filename) {
  object3dRenderer_ = object3dRenderer;
  srvManager_ = srvManager;

  // モデルデータの読み込み
  ModelManager::GetInstance()->LoadModel(directoryPath + "/" + filename);

  model_ = std::make_unique<Model>();
  model_->Initialize(ModelManager::GetInstance()->GetModelRenderer(),
                     directoryPath, filename);

  // アニメーションの読み込み
  animation_ = LoadAnimationFile(directoryPath, filename);

  // スケルトンの作成
  skeleton_ = CreateSkeleton(model_->GetRootNode());

  // スキンクラスターの作成
  skinCluster_ = CreateSkinCluster(object3dRenderer->GetDx12Core(), srvManager,
                                   skeleton_, model_.get());

  // 描画用Object3dの作成と割り当て
  object3d_ = std::make_unique<Object3d>();
  object3d_->Initialize(object3dRenderer);
  object3d_->SetModel(model_.get());
  object3d_->SetSkinCluster(&skinCluster_);
}

void SkinnedObject::Update(float deltaTime) {
  // アニメーション時間の更新
  animationTime_ += deltaTime;
  animationTime_ = std::fmod(animationTime_, animation_.duration);

  // アニメーションの適用（各関節のローカル行列更新）
  ApplyAnimation(skeleton_, animation_, animationTime_);

  // スケルトンの更新（親から子への行列伝播）
  ::Update(skeleton_);

  // スキンクラスターの更新（パレットの更新）
  ::Update(skinCluster_, skeleton_);

  // ComputeShaderを実行してスキニング計算
  if (object3dRenderer_ && srvManager_ && model_) {
    auto commandList = object3dRenderer_->GetDx12Core()->GetCommandList();

    // DescriptorHeapのセット
    srvManager_->PreDraw();

    // ResourceBarrier: COMMON or VBV -> UNORDERED_ACCESS
    D3D12_RESOURCE_BARRIER barrierUAV{};
    barrierUAV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierUAV.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierUAV.Transition.pResource = skinCluster_.skinnedVertexResource.Get();
    barrierUAV.Transition.StateBefore =
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; // 描画後の状態から遷移させる
    barrierUAV.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrierUAV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrierUAV);

    commandList->SetComputeRootSignature(
        object3dRenderer_->GetSkinningComputeRootSignature());
    commandList->SetPipelineState(
        object3dRenderer_->GetSkinningComputePipelineState());

    commandList->SetComputeRootDescriptorTable(
        0, skinCluster_.paletteSrvHandle.second);
    commandList->SetComputeRootDescriptorTable(
        1, skinCluster_.inputVertexSrvHandle.second);
    commandList->SetComputeRootDescriptorTable(
        2, skinCluster_.influenceSrvHandle.second);
    commandList->SetComputeRootDescriptorTable(
        3, skinCluster_.skinnedVertexUavHandle.second);
    commandList->SetComputeRootConstantBufferView(
        4, skinCluster_.skinningInformationResource->GetGPUVirtualAddress());

    UINT numVertices =
        static_cast<UINT>(model_->GetModelData().vertices.size());
    commandList->Dispatch((numVertices + 1023) / 1024, 1, 1);

    // ResourceBarrier: UNORDERED_ACCESS -> VBV
    D3D12_RESOURCE_BARRIER barrierVBV{};
    barrierVBV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierVBV.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierVBV.Transition.pResource = skinCluster_.skinnedVertexResource.Get();
    barrierVBV.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrierVBV.Transition.StateAfter =
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barrierVBV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrierVBV);
  }

  // 3Dオブジェクトの全体更新（WVP行列などの計算）
  object3d_->Update();
}

void SkinnedObject::Draw() {
  if (object3d_) {
    object3d_->Draw();
  }
}
