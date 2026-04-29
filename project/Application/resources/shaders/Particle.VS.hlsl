#include "Particle.hlsli"

struct Particle {
    float32_t3 translate;
    float32_t padding1;
    float32_t3 scale;
    float32_t padding2;
    float32_t3 rotate;
    float32_t lifeTime;
    float32_t3 velocity;
    float32_t currentTime;
    float32_t4 color;
    float32_t3 scaleVelocity;
    float32_t padding3;
};

struct PerView {
    float32_t4x4 viewProjection;
    float32_t4x4 billboardMatrix;
};

StructuredBuffer<Particle> gParticles : register(t0);
ConstantBuffer<PerView> gPerView : register(b0);
ConstantBuffer<Material> gMaterial : register(b1);

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

float32_t4x4 MakeRotateXMatrix(float32_t theta) {
    float32_t c = cos(theta);
    float32_t s = sin(theta);
    return float32_t4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c, s, 0.0f,
        0.0f, -s, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

float32_t4x4 MakeRotateYMatrix(float32_t theta) {
    float32_t c = cos(theta);
    float32_t s = sin(theta);
    return float32_t4x4(
        c, 0.0f, -s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        s, 0.0f, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

float32_t4x4 MakeRotateZMatrix(float32_t theta) {
    float32_t c = cos(theta);
    float32_t s = sin(theta);
    return float32_t4x4(
        c, s, 0.0f, 0.0f,
        -s, c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceId : SV_InstanceID) {
    VertexShaderOutput output;
    
    Particle particle = gParticles[instanceId];
    
    float32_t4x4 worldMatrix;
    
    if (gMaterial.isBillboard != 0) {
        // billboardMatrixを元にworldMatrixを構築
        worldMatrix = gPerView.billboardMatrix;
        worldMatrix[0] *= particle.scale.x;
        worldMatrix[1] *= particle.scale.y;
        worldMatrix[2] *= particle.scale.z;
        worldMatrix[3].xyz = particle.translate;
    } else {
        // 回転・スケール・平行移動を使った標準のアフィン変換
        float32_t4x4 scaleMatrix = float32_t4x4(
            particle.scale.x, 0.0f, 0.0f, 0.0f,
            0.0f, particle.scale.y, 0.0f, 0.0f,
            0.0f, 0.0f, particle.scale.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
        
        float32_t4x4 rotateMatrix = mul(MakeRotateZMatrix(particle.rotate.z), mul(MakeRotateXMatrix(particle.rotate.x), MakeRotateYMatrix(particle.rotate.y)));
        
        float32_t4x4 translateMatrix = float32_t4x4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            particle.translate.x, particle.translate.y, particle.translate.z, 1.0f
        );
        
        worldMatrix = mul(scaleMatrix, mul(rotateMatrix, translateMatrix));
    }

    output.position = mul(input.position, mul(worldMatrix, gPerView.viewProjection));
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    output.texcoord = transformedUV.xy;
    output.color = particle.color;
    
    return output;
}
