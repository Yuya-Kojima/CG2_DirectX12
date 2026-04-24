#include "Particle/IParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include "Texture/TextureManager.h"
#include "Debug/Logger.h"
#include "Math/MathUtil.h"
#include "Math/MathUtil.h"

void IParticleEmitter::BaseInitialize(const std::string& textureFilePath) {
    textureFilePath_ = textureFilePath;
    TextureManager::GetInstance()->LoadTexture(textureFilePath_);
    textureSrvIndex_ = TextureManager::GetInstance()->GetSrvIndex(textureFilePath_);

    randomEngine_ = std::mt19937(seedGenerator_());

    CreateInstancingResource();
    CreateMaterialResource();
}

void IParticleEmitter::CreateInstancingResource() {
    auto dx12Core = ParticleManager::GetInstance()->GetDx12Core();
    auto srvManager = ParticleManager::GetInstance()->GetSrvManager();

    instancingResource_ = dx12Core->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance_);

    instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));

    for (uint32_t index = 0; index < kNumMaxInstance_; ++index) {
        instancingData_[index].WVP = MakeIdentity4x4();
        instancingData_[index].World = MakeIdentity4x4();
        instancingData_[index].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    instancingSrvIndex_ = srvManager->Allocate();

    srvManager->CreateSRVforStructuredBuffer(
        instancingSrvIndex_, instancingResource_.Get(),
        kNumMaxInstance_, sizeof(ParticleForGPU));
}

void IParticleEmitter::CreateMaterialResource() {
    auto dx12Core = ParticleManager::GetInstance()->GetDx12Core();

    materialResource_ = dx12Core->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = 0;
    materialData_->uvTransform = MakeIdentity4x4();
}

void IParticleEmitter::ClearParticles() {
    particles_.clear();
    numInstance_ = 0;
}

void IParticleEmitter::Emit(const ParticleEmitDesc& desc) {
    auto randSym = [&](float halfRange) -> float {
        if (halfRange <= 0.0f) return 0.0f;
        std::uniform_real_distribution<float> d(-halfRange, +halfRange);
        return d(randomEngine_);
    };

    float lifeMin = desc.lifeMin;
    float lifeMax = desc.lifeMax;
    if (lifeMin > lifeMax) std::swap(lifeMin, lifeMax);
    std::uniform_real_distribution<float> distLife(lifeMin, lifeMax);

    for (uint32_t i = 0; i < desc.count; ++i) {
        Particle particle{};

        particle.transform.translate = {
            desc.position.x + randSym(desc.spawnRandom.x),
            desc.position.y + randSym(desc.spawnRandom.y),
            desc.position.z + randSym(desc.spawnRandom.z),
        };

        particle.transform.scale = {
            desc.baseScale.x + randSym(desc.scaleRandom.x),
            desc.baseScale.y + randSym(desc.scaleRandom.y),
            desc.baseScale.z + randSym(desc.scaleRandom.z),
        };

        particle.transform.scale.x = (std::max)(0.0001f, particle.transform.scale.x);
        particle.transform.scale.y = (std::max)(0.0001f, particle.transform.scale.y);
        particle.transform.scale.z = (std::max)(0.0001f, particle.transform.scale.z);

        particle.transform.rotate = {
            desc.baseRotate.x + randSym(desc.rotateRandom.x),
            desc.baseRotate.y + randSym(desc.rotateRandom.y),
            desc.baseRotate.z + randSym(desc.rotateRandom.z),
        };

        particle.velocity = {
            desc.baseVelocity.x + randSym(desc.velocityRandom.x),
            desc.baseVelocity.y + randSym(desc.velocityRandom.y),
            desc.baseVelocity.z + randSym(desc.velocityRandom.z),
        };

        particle.color = desc.color;
        particle.lifeTime = distLife(randomEngine_);
        particle.currentTime = 0.0f;

        particles_.push_back(particle);
    }
}

void IParticleEmitter::Emit(const Vector3& position, uint32_t count,
    const Vector3& baseVelocity, const Vector3& velocityRandom,
    const Vector3& spawnRandom, float lifeMin, float lifeMax,
    const Vector4& color, const Vector3& baseScale, const Vector3& scaleRandom,
    const Vector3& baseRotate, const Vector3& rotateRandom) {
    
    ParticleEmitDesc desc{};
    desc.position = position;
    desc.count = count;
    desc.baseVelocity = baseVelocity;
    desc.velocityRandom = velocityRandom;
    desc.spawnRandom = spawnRandom;
    desc.lifeMin = lifeMin;
    desc.lifeMax = lifeMax;
    desc.color = color;
    desc.baseScale = baseScale;
    desc.scaleRandom = scaleRandom;
    desc.baseRotate = baseRotate;
    desc.rotateRandom = rotateRandom;

    Emit(desc);
}

void IParticleEmitter::Draw() {
    if (numInstance_ == 0 || vertexCount_ == 0) return;

    auto dx12Core = ParticleManager::GetInstance()->GetDx12Core();
    auto srvManager = ParticleManager::GetInstance()->GetSrvManager();
    auto commandList = dx12Core->GetCommandList();

    commandList->SetGraphicsRootSignature(ParticleManager::GetInstance()->GetRootSignature());
    commandList->SetPipelineState(ParticleManager::GetInstance()->GetPipelineState());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU =
        srvManager->GetGPUDescriptorHandle(instancingSrvIndex_);
    commandList->SetGraphicsRootDescriptorTable(1, instancingSrvHandleGPU);

    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU =
        srvManager->GetGPUDescriptorHandle(textureSrvIndex_);
    commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

    commandList->DrawInstanced(vertexCount_, numInstance_, 0, 0);
}
