#pragma once

#include "Math/MathUtil.h"
#include <string>

class ParticleEmitter {

public:
	ParticleEmitter(const std::string& name, Vector3 center, Vector3 halfSize,
		int count, float frequency, Vector3 baseVel,
		Vector3 velRandom, float lifeMin, float lifeMax);

	void Update();

	void Emit();

	void SetCenter(const Vector3& center) { center_ = center; }

	void SetHalfSize(const Vector3& halfSize);

	void SetCount(int count);
	void SetFrequency(float frequency);

	void SetBaseVelocity(const Vector3& v) { baseVelocity_ = v; }
	void SetVelocityRandom(const Vector3& v) { velocityRand_ = v; }

	void SetLifeRange(float lifeMin, float lifeMax);

	void SetColor(const Vector4& color) { color_ = color; }

private:
	std::string name_;
	Vector3 center_;   // 中心座標
	Vector3 halfSize_; // 発生範囲(AABB)
	int count_;        // 発生する数
	float frequency_;  // 発生間隔
	float frequencyTime_ = 0.0f;

	Vector3 baseVelocity_{};
	Vector3 velocityRand_{};

	float lifeMin_;
	float lifeMax_;

	Vector4 color_ = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
};
