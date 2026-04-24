#pragma once
#include "Core/Dx12Core.h"
#include "Core/SrvManager.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector4.h"
#include "Particle/Particle.h"
#include <string>
#include <list>
#include <random>
#include <wrl.h>

struct ParticleEmitDesc {
    // グループ名は各インスタンスで管理するため削除
    Vector3 position{}; // center
    uint32_t count = 1; // 発生数

    Vector3 baseVelocity{};   // ベース速度
    Vector3 velocityRandom{}; // 速度ばらつき(±)
    Vector3 spawnRandom{};    // 生成位置ばらつき(±)

    float lifeMin = 1.0f;
    float lifeMax = 1.0f;

    Vector4 color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }; // 白

    Vector3 baseScale{ 1.0f, 1.0f, 1.0f };
    Vector3 scaleRandom{};
    Vector3 baseRotate{};
    Vector3 rotateRandom{};
};

struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
};

class IParticleEmitter {
public:
    IParticleEmitter() = default;
    virtual ~IParticleEmitter() = default;

    virtual void Emit(const ParticleEmitDesc& desc);
    virtual void Emit(const Vector3& position, uint32_t count,
        const Vector3& baseVelocity, const Vector3& velocityRandom,
        const Vector3& spawnRandom, float lifeMin, float lifeMax,
        const Vector4& color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f },
        const Vector3& baseScale = Vector3{ 1.0f, 1.0f, 1.0f },
        const Vector3& scaleRandom = Vector3{},
        const Vector3& baseRotate = Vector3{},
        const Vector3& rotateRandom = Vector3{});

    virtual void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) = 0;
    virtual void Draw();

    void ClearParticles();

protected:
    void BaseInitialize(const std::string& textureFilePath);
    void CreateInstancingResource();
    void CreateMaterialResource();

    std::string textureFilePath_;
    uint32_t textureSrvIndex_ = 0;

    std::list<Particle> particles_;

    uint32_t instancingSrvIndex_ = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
    ParticleForGPU* instancingData_ = nullptr;
    uint32_t numInstance_ = 0;
    
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    uint32_t vertexCount_ = 0;

    struct Material {
        Vector4 color;
        int32_t enableLighting;
        float padding[3];
        Matrix4x4 uvTransform;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

    const uint32_t kNumMaxInstance_ = 100;

    std::random_device seedGenerator_;
    std::mt19937 randomEngine_;
};
