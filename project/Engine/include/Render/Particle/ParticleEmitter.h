#pragma once

#include "Math/Transform.h"
#include "Math/Vector3.h"
#include <string> 

class ParticleEmitter {

public:

	ParticleEmitter(const std::string& name, Transform transform, int count, float frequency, float frequencyTime);

	void Update();

	void Emit();

private:
	std::string name_;
	Transform transform_; // エミッターのtransform
	int count_;      // 発生する数
	float frequency_;     // 発生頻度
	float frequencyTime_;

};

