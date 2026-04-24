#include "Particle/MeshParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include "Math/MathUtil.h"
#include "Render/Primitive/Ring.h"
#include "Render/Primitive/Cylinder.h"
#include <cstring>

void MeshParticleEmitter::InitializeAsPlane(const std::string& textureFilePath) {
    BaseInitialize(textureFilePath);
    CreateVertexDataFromPlane();
}

void MeshParticleEmitter::InitializeAsRing(const std::string& textureFilePath, 
    uint32_t divide, float outerRadius, float innerRadius) {
    BaseInitialize(textureFilePath);
    
    Ring ring;
    ring.Build(divide, outerRadius, innerRadius);
    CreateVertexDataFromVertices(ring.GetVertices());
}

void MeshParticleEmitter::InitializeAsCylinder(const std::string& textureFilePath, 
    uint32_t divide, float topRadius, float bottomRadius, float height) {
    BaseInitialize(textureFilePath);
    
    Cylinder cylinder;
    cylinder.Build(divide, topRadius, bottomRadius, height);
    CreateVertexDataFromVertices(cylinder.GetVertices());
}

void MeshParticleEmitter::CreateVertexDataFromPlane() {
    auto dx12Core = ParticleManager::GetInstance()->GetDx12Core();

    vertexResource_ = dx12Core->CreateBufferResource(sizeof(VertexData) * 6);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    vertexData[0].position = { -1.0f, 1.0f, 0.0f, 1.0f };
    vertexData[0].texcoord = { 0.0f, 0.0f };
    vertexData[0].normal = { 0.0f, 0.0f, 1.0f };

    vertexData[1].position = { 1.0f, 1.0f, 0.0f, 1.0f };
    vertexData[1].texcoord = { 1.0f, 0.0f };
    vertexData[1].normal = { 0.0f, 0.0f, 1.0f };

    vertexData[2].position = { -1.0f, -1.0f, 0.0f, 1.0f };
    vertexData[2].texcoord = { 0.0f, 1.0f };
    vertexData[2].normal = { 0.0f, 0.0f, 1.0f };

    vertexData[3].position = { -1.0f, -1.0f, 0.0f, 1.0f };
    vertexData[3].texcoord = { 0.0f, 1.0f };
    vertexData[3].normal = { 0.0f, 0.0f, 1.0f };

    vertexData[4].position = { 1.0f, 1.0f, 0.0f, 1.0f };
    vertexData[4].texcoord = { 1.0f, 0.0f };
    vertexData[4].normal = { 0.0f, 0.0f, 1.0f };

    vertexData[5].position = { 1.0f, -1.0f, 0.0f, 1.0f };
    vertexData[5].texcoord = { 1.0f, 1.0f };
    vertexData[5].normal = { 0.0f, 0.0f, 1.0f };

    vertexCount_ = 6;
}

void MeshParticleEmitter::CreateVertexDataFromVertices(const std::vector<VertexData>& vertices) {
    auto dx12Core = ParticleManager::GetInstance()->GetDx12Core();
    
    vertexCount_ = static_cast<uint32_t>(vertices.size());
    if (vertexCount_ == 0) return;

    vertexResource_ = dx12Core->CreateBufferResource(sizeof(VertexData) * vertices.size());
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* dst = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&dst));
    std::memcpy(dst, vertices.data(), sizeof(VertexData) * vertices.size());
}

void MeshParticleEmitter::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
    const float deltaTime = 1.0f / 60.0f;
    uint32_t instanceIndex = 0;

    for (auto it = particles_.begin(); it != particles_.end();) {
        it->currentTime += deltaTime;

        if (it->currentTime >= it->lifeTime) {
            it = particles_.erase(it);
            continue;
        }

        it->transform.translate += it->velocity * deltaTime;

        float t = 0.0f;
        if (it->lifeTime > 0.0f) {
            t = it->currentTime / it->lifeTime;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
        }

        Vector4 c = it->color;
        c.w = it->color.w * (1.0f - t);

        if (instanceIndex < kNumMaxInstance_) {
            Matrix4x4 scaleMatrix = MakeScaleMatrix(it->transform.scale);
            
            // XYZすべての回転を適用する（ビルボード行列は使わない）
            Matrix4x4 rotateXMatrix = MakeRotateXMatrix(it->transform.rotate.x);
            Matrix4x4 rotateYMatrix = MakeRotateYMatrix(it->transform.rotate.y);
            Matrix4x4 rotateZMatrix = MakeRotateZMatrix(it->transform.rotate.z);
            Matrix4x4 rotateMatrix = Multiply(Multiply(rotateXMatrix, rotateYMatrix), rotateZMatrix);

            Matrix4x4 translateMatrix = MakeTranslateMatrix(it->transform.translate);

            Matrix4x4 worldMatrix = Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);
            Matrix4x4 worldViewProjectionMatrix = Multiply(Multiply(worldMatrix, viewMatrix), projectionMatrix);

            instancingData_[instanceIndex].World = worldMatrix;
            instancingData_[instanceIndex].WVP = worldViewProjectionMatrix;
            instancingData_[instanceIndex].color = c;

            instanceIndex++;
        }
        ++it;
    }
    numInstance_ = instanceIndex;
}
