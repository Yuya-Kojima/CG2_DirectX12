#include "Render/ParticleEmitter.h"
#include"Render/ParticleManager.h"

ParticleEmitter::ParticleEmitter(const std::string& name, Transform transform, int count, float frequency, float frequencyTime) {

	name_ = name;

	transform_ = transform;

	count_ = count;

	frequency_ = frequency;

	frequencyTime_ = frequencyTime;

}

void ParticleEmitter::Update() {

	const float deltaTime = 1.0f / 60.0f;


	frequencyTime_ += deltaTime;

	if (frequencyTime_ >= frequency_) {
		Emit();
		frequencyTime_ -= frequency_;
	}

}

void ParticleEmitter::Emit() {
	ParticleManager::GetInstance()->Emit(name_, transform_.translate, static_cast<uint32_t>(count_));
}