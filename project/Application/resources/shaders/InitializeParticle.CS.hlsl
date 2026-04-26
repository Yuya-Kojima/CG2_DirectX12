#include "Particle.CS.hlsl"

static const uint32_t kMaxParticles = 1024;

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeCounter : register(u1);

[numthreads(1024, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID) {
    uint32_t particleIndex = DTid.x;
    
    if (particleIndex == 0) {
        gFreeCounter[0] = 0;
    }

    if (particleIndex < kMaxParticles) {
        // Particle構造体の全要素を0で埋める
        gParticles[particleIndex] = (Particle)0;
    }
}
