#pragma once
#include "Particle/IParticleEmitter.h"

class BillboardParticleEmitter : public IParticleEmitter {
public:
    BillboardParticleEmitter() = default;
    ~BillboardParticleEmitter() override = default;

    void Initialize(const std::string& textureFilePath);

    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) override;

private:
    void CreateVertexData();
};
