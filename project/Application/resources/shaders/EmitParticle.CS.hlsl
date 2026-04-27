#include "Particle.CS.hlsl"

struct EmitterSphere {
    float32_t3 translate; // 位置
    float32_t radius;     // 射出半径
    uint32_t count;       // 射出数
    float32_t frequency;  // 射出間隔
    float32_t frequencyTime; // 射出間隔調整用時間
    uint32_t emit;        // 射出許可
};

struct PerFrame {
    float32_t time;
    float32_t deltaTime;
};

ConstantBuffer<EmitterSphere> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeCounter : register(u1);

static const uint32_t kMaxParticles = 1024;

// --- 疑似乱数生成関数 ---
// 1D用の疑似乱数
float32_t rand3dTo1d(float32_t3 value, float32_t3 dotDir = float32_t3(12.9898, 78.233, 37.719)) {
    float32_t3 smallValue = sin(value);
    float32_t random = dot(smallValue, dotDir);
    return frac(sin(random) * 143758.5453);
}

// 3D用の疑似乱数
float32_t3 rand3dTo3d(float32_t3 value) {
    return float32_t3(
        rand3dTo1d(value, float32_t3(12.989, 78.233, 37.719)),
        rand3dTo1d(value, float32_t3(39.346, 11.135, 83.155)),
        rand3dTo1d(value, float32_t3(73.156, 52.235, 09.151))
    );
}

// classにしているがすべてpublic扱いになる
class RandomGenerator {
    float32_t3 seed;

    float32_t3 Generate3d() {
        seed = rand3dTo3d(seed);
        return seed;
    }

    float32_t Generate1d() {
        float32_t result = rand3dTo1d(seed);
        seed.x = result;
        return result;
    }
};

[numthreads(1, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID) {
    if (gEmitter.emit != 0) { // 射出許可が出たので射出
        
        RandomGenerator generator;
        // DTid と時間を使って乱数のシードを作る
        generator.seed = (DTid + gPerFrame.time) * gPerFrame.time;

        for (uint32_t countIndex = 0; countIndex < gEmitter.count; ++countIndex) {
            
            int32_t particleIndex;
            // gFreeCounter[0]に1を足し、足す前の値をparticleIndexに格納する
            InterlockedAdd(gFreeCounter[0], 1, particleIndex);

            // 最大数よりもparticleの数が少なければ射出可能
            if (particleIndex < kMaxParticles) {
                // 0.0〜1.0 の乱数を取得し、いい感じに調整する
                float32_t3 randomScale = generator.Generate3d();
                float32_t3 randomTranslate = generator.Generate3d();
                float32_t3 randomColor = generator.Generate3d();

                // Particleの初期化
                // Scaleは 0.1 ~ 0.5 ぐらいに調整
                gParticles[particleIndex].scale = float32_t3(0.1f, 0.1f, 0.1f) + randomScale * 0.4f;
                
                // Translateは -1.0 ~ 1.0 に調整 (Emitterの位置も足す)
                gParticles[particleIndex].translate = gEmitter.translate + (randomTranslate - 0.5f) * 2.0f;
                
                // Colorはランダム色、alphaは1.0
                gParticles[particleIndex].color.rgb = randomColor;
                gParticles[particleIndex].color.a = 1.0f;

                // 速度 (velocity) をランダムに設定 (-0.05 ~ 0.05 の範囲で調整)
                float32_t3 randomVelocity = generator.Generate3d();
                gParticles[particleIndex].velocity = (randomVelocity - 0.5f) * 0.1f;

                // 寿命 (lifeTime) をランダムに設定 (1.0秒 ~ 3.0秒)
                gParticles[particleIndex].lifeTime = 1.0f + generator.Generate1d() * 2.0f;

                // 経過時間 (currentTime) を 0 に初期化
                gParticles[particleIndex].currentTime = 0.0f;
            }
        }
    }
}
