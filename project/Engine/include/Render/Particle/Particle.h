#pragma once
#include "Math/Transform.h"
#include "Math/Vector3.h"
#include <random>

struct Particle {
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

struct Emitter {
	Transform transform; // エミッターのtransform
	uint32_t count;      // 発生する数
	float frequency;     // 発生頻度
	float frequencyTime;
};

//inline Particle MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate) {
//
//	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
//
//	Particle particle;
//	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
//
//	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
//
//	Vector3 randomTranslate{
//		distribution(randomEngine),
//		distribution(randomEngine),
//		distribution(randomEngine),
//	};
//
//	particle.transform.translate = translate + randomTranslate;
//
//	// particle.transform.translate = {
//	//     distribution(randomEngine),
//	//     distribution(randomEngine),
//	//     distribution(randomEngine),
//	// };
//
//	particle.velocity = {
//		distribution(randomEngine),
//		distribution(randomEngine),
//		distribution(randomEngine),
//	};
//
//	particle.color = {
//		distribution(randomEngine),
//		distribution(randomEngine),
//		distribution(randomEngine),
//		1.0f,
//	};
//
//	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
//	particle.lifeTime = distTime(randomEngine);
//	particle.currentTime = 0;
//
//	return particle;
//}
//
//inline std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randomEngine) {
//	std::list<Particle> particles;
//
//	for (uint32_t count = 0; count < emitter.count; ++count) {
//		particles.push_back(
//			MakeNewParticle(randomEngine, emitter.transform.translate));
//	}
//
//	return particles;
//}