#include "Render/Model/SkinCluster.h"
#include "Render/Model/Skeleton.h"
#include "Core/Dx12Core.h"
#include "Core/SrvManager.h"
#include <algorithm>

SkinCluster CreateSkinCluster(Dx12Core* dx12Core, SrvManager* srvManager, const Skeleton& skeleton, const Model::ModelData& modelData) {
    SkinCluster skinCluster;

    // 1. palette用のResourceを確保
    skinCluster.paletteResource = dx12Core->CreateBufferResource(sizeof(WellForGPU) * skeleton.joints.size());
    WellForGPU* mappedPalette = nullptr;
    skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
    skinCluster.mappedPalette = { mappedPalette, skeleton.joints.size() };

    // 2. palette用のsrvを作成 (StructuredBuffer)
    uint32_t srvIndex = srvManager->Allocate();
    skinCluster.paletteSrvHandle.first = srvManager->GetCPUDescriptorHandle(srvIndex);
    skinCluster.paletteSrvHandle.second = srvManager->GetGPUDescriptorHandle(srvIndex);
    srvManager->CreateSRVforStructuredBuffer(srvIndex, skinCluster.paletteResource.Get(), static_cast<UINT>(skeleton.joints.size()), sizeof(WellForGPU));

    // 3. influence用のResourceを確保
    skinCluster.influenceResource = dx12Core->CreateBufferResource(sizeof(VertexInfluence) * modelData.vertices.size());
    VertexInfluence* mappedInfluence = nullptr;
    skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
    std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size()); // 0埋め。weightを0にしておく。
    skinCluster.mappedInfluence = { mappedInfluence, modelData.vertices.size() };

    // 4. Influence用のVBVを作成
    skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
    skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
    skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

    // 5. InverseBindPoseMatrixの保存領域を作成して、単位行列で埋める
    skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
    std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), MakeIdentity4x4);

    // 6. ModelDataのSkinCluster情報を解析してInfluenceの中身を埋める
    for (const auto& jointWeight : modelData.skinClusterData) {
        auto it = skeleton.jointMap.find(jointWeight.first);
        if (it == skeleton.jointMap.end()) {
            continue; // そんな名前のJointは存在しないので次に回す
        }

        // (*it).secondにはjointのindexが入っているので、該当のindexのinverseBindPoseMatrixを代入
        skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;

        for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
            auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex]; // 該当のvertexIndexのinfluence情報を参照しておく
            for (uint32_t index = 0; index < kNumMaxInfluence; ++index) {
                if (currentInfluence.weights[index] == 0.0f) { // weight==0が空いている状態なので、その場所にweightとjointのindexを代入
                    currentInfluence.weights[index] = vertexWeight.weight;
                    currentInfluence.jointIndices[index] = (*it).second;
                    break;
                }
            }
        }
    }

    return skinCluster;
}

void Update(SkinCluster& skinCluster, const Skeleton& skeleton) {
    for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
        assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
        
        // パレットの位置用行列を計算： 初期姿勢の逆行列 * 現在のアニメーション姿勢行列
        skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix =
            Multiply(skinCluster.inverseBindPoseMatrices[jointIndex], skeleton.joints[jointIndex].skeletonSpaceMatrix);
        
        // パレットの法線用行列を計算： 位置用行列の逆転置行列
        skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
            Transpose(Inverse(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix));
    }
}
