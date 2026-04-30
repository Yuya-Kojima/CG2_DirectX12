#include "Particle.CS.hlsl"

struct EmitterData {
	float32_t3 translate; // 12 bytes
	float32_t padding0; // 4 bytes  (16 bytes)

	float32_t3 spawnArea; // 12 bytes
	float32_t padding1; // 4 bytes  (16 bytes)

	uint32_t count; // 4 bytes
	float32_t frequency; // 4 bytes
	float32_t frequencyTime; // 4 bytes
	uint32_t emit; // 4 bytes  (16 bytes)

	float32_t3 baseColor; // 12 bytes
	float32_t lifeMin; // 4 bytes  (16 bytes)

	float32_t3 baseVelocity; // 12 bytes
	float32_t lifeMax; // 4 bytes  (16 bytes)

	float32_t3 velocityRandom; // 12 bytes
	float32_t padding2; // 4 bytes  (16 bytes)

	float32_t3 baseScale; // 12 bytes
	float32_t padding3; // 4 bytes  (16 bytes)

	float32_t3 scaleRandom; // 12 bytes
	float32_t padding4; // 4 bytes  (16 bytes)

	float32_t3 baseRotate; // 12 bytes
	float32_t padding5; // 4 bytes  (16 bytes)

	float32_t3 rotateRandom; // 12 bytes
	float32_t padding6; // 4 bytes  (16 bytes)

	float32_t3 scaleVelocity; // 12 bytes
	float32_t padding7; // 4 bytes  (16 bytes)
};

struct PerFrame {
	float32_t time;
	float32_t deltaTime;
};

ConstantBuffer<EmitterData> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeListIndex : register(u1);
RWStructuredBuffer<uint32_t> gFreeList : register(u2);

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

struct RandomGenerator {
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
	RandomGenerator generator;
    // DTid と時間を使って乱数のシードを作る
	generator.seed = (DTid + gPerFrame.time) * gPerFrame.time;

	for (uint32_t countIndex = 0; countIndex < gEmitter.count; ++countIndex) {
        
		int32_t freeListIndex;
        // FreeListのIndexを1つ前に設定し、現在のIndexを取得する
		InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);

		if (0 <= freeListIndex && freeListIndex < kMaxParticles) {
			uint32_t particleIndex = gFreeList[freeListIndex];

            // 0.0〜1.0 の乱数を取得し、いい感じに調整する
			float32_t3 randomScale = generator.Generate3d();
			float32_t3 randomTranslate = generator.Generate3d();
			float32_t3 randomVelocity = generator.Generate3d();

            // Particleの初期化
            // Scaleは baseScale + scaleRandom * (0.0~1.0)
			gParticles[particleIndex].scale = gEmitter.baseScale + randomScale * gEmitter.scaleRandom;
            
            // Rotateは baseRotate + rotateRandom * (-1.0~1.0)
			float32_t3 randomRotate = generator.Generate3d();
			gParticles[particleIndex].rotate = gEmitter.baseRotate + (randomRotate - 0.5f) * 2.0f * gEmitter.rotateRandom;

            // Translateは エミッター位置を中心に、spawnAreaの範囲で散らす
            // (randomTranslate - 0.5f) * 2.0f は -1.0 ~ 1.0 になる
			gParticles[particleIndex].translate = gEmitter.translate + (randomTranslate - 0.5f) * 2.0f * gEmitter.spawnArea;
            
            // Colorは baseColor、alphaは1.0
			gParticles[particleIndex].color.rgb = gEmitter.baseColor;
			gParticles[particleIndex].color.a = 1.0f;

            // 速度 (velocity) をランダムに設定
            // baseVelocity + velocityRandom * (-1.0 ~ 1.0)
			gParticles[particleIndex].velocity = gEmitter.baseVelocity + (randomVelocity - 0.5f) * 2.0f * gEmitter.velocityRandom;

            // 寿命 (lifeTime) をランダムに設定
			gParticles[particleIndex].lifeTime = gEmitter.lifeMin + generator.Generate1d() * (gEmitter.lifeMax - gEmitter.lifeMin);

            // 経過時間 (currentTime) を 0 に初期化
			gParticles[particleIndex].currentTime = 0.0f;
			gParticles[particleIndex].color.rgb = gEmitter.baseColor;
			gParticles[particleIndex].color.a = 1.0f;
			gParticles[particleIndex].scaleVelocity = gEmitter.scaleVelocity;
		} else {
            // 発生させられなかったので、減らしてしまった分もとに戻す。
			InterlockedAdd(gFreeListIndex[0], 1);
            // Emit中にParticleは消えないので、この後発生することはないためbreakして終わらせる
			break;
		}
	}
}
