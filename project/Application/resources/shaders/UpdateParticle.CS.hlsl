#include "Particle.CS.hlsl"

struct PerFrame {
    float32_t time;
    float32_t deltaTime;
};

ConstantBuffer<PerFrame> gPerFrame : register(b0);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeListIndex : register(u1);
RWStructuredBuffer<uint32_t> gFreeList : register(u2);

static const uint32_t kMaxParticles = 1024;

[numthreads(1024, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID) {
    uint32_t particleIndex = DTid.x;
    if (particleIndex < kMaxParticles) {
        // alphaが0のparticleは死んでいるとみなして更新しない
        if (gParticles[particleIndex].color.a != 0.0f) {
            gParticles[particleIndex].translate += gParticles[particleIndex].velocity;
            gParticles[particleIndex].currentTime += gPerFrame.deltaTime;
            
            float32_t alpha = 1.0f - (gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime);
            gParticles[particleIndex].color.a = saturate(alpha);

            // alphaが0になったので、ここはFreeとする
            if (gParticles[particleIndex].color.a == 0.0f) {
                // スケールに0を入れておいてVertexShader出力で棄却されるようにする
                gParticles[particleIndex].scale = float32_t3(0.0f, 0.0f, 0.0f);
                int32_t freeListIndex;
                InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
                // 最新のFreeListIndexの場所に死んだParticleのIndexを設定する。
                if ((freeListIndex + 1) < kMaxParticles) {
                    gFreeList[freeListIndex + 1] = particleIndex;
                } else {
                    // ここにくるはずはない。きたら何かが間違っているが、安全策をうっておく
                    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
                }
            }
        }
    }
}
