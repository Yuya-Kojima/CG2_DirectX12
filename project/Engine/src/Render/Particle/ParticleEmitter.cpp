#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include <algorithm>
#include <cmath>

ParticleEmitter::ParticleEmitter(const std::string &name, Vector3 center,
                                 Vector3 halfSize, int count, float frequency,
                                 Vector3 baseVel, Vector3 velRandom,
                                 float lifeMin, float lifeMax) {

  name_ = name;
  center_ = center;

  frequencyTime_ = 0.0f;

  SetHalfSize(halfSize);
  SetCount(count);
  SetFrequency(frequency);
  SetBaseVelocity(baseVel);
  SetVelocityRandom(velRandom);
  SetLifeRange(lifeMin, lifeMax);
}

void ParticleEmitter::SetHalfSize(const Vector3 &halfSize) {
  halfSize_.x = (std::max)(0.0f, halfSize.x);
  halfSize_.y = (std::max)(0.0f, halfSize.y);
  halfSize_.z = (std::max)(0.0f, halfSize.z);
}

void ParticleEmitter::SetCount(int count) { count_ = (std::max)(0, count); }

void ParticleEmitter::SetFrequency(float frequency) {
  if (!std::isfinite(frequency) || frequency <= 0.0f) {
    frequency_ = 0.0f;
    frequencyTime_ = 0.0f;
    return;
  }
  frequency_ = frequency;

  if (!std::isfinite(frequencyTime_) || frequencyTime_ < 0.0f) {
    frequencyTime_ = 0.0f;
  }
}

void ParticleEmitter::SetLifeRange(float lifeMin, float lifeMax) {
  if (!std::isfinite(lifeMin)) {
    lifeMin = 0.0f;
  }
  if (!std::isfinite(lifeMax)) {
    lifeMax = 0.0f;
  }

  lifeMin = (std::max)(0.0f, lifeMin);
  lifeMax = (std::max)(0.0f, lifeMax);

  if (lifeMin > lifeMax) {
    std::swap(lifeMin, lifeMax);
  }

  constexpr float kMinLife = 0.01f;
  if (lifeMax < kMinLife) {
    lifeMax = kMinLife;
  }
  if (lifeMin < kMinLife) {
    lifeMin = kMinLife;
  }

  lifeMin_ = lifeMin;
  lifeMax_ = lifeMax;
}

void ParticleEmitter::Update() {

  const float deltaTime = 1.0f / 60.0f;

  // frequencyが無効なら自動発生しない
  if (frequency_ <= 0.0f || count_ <= 0) {
    return;
  }

  frequencyTime_ += deltaTime;

  while (frequencyTime_ >= frequency_) {
    Emit();
    frequencyTime_ -= frequency_;
  }
}

void ParticleEmitter::Emit() {
  if (count_ <= 0) {
    return;
  }

  ParticleManager::ParticleEmitDesc desc{};
  desc.name = name_;
  desc.position = center_;
  desc.count = static_cast<uint32_t>(count_);
  desc.baseVelocity = baseVelocity_;
  desc.velocityRandom = velocityRand_;
  desc.spawnRandom = halfSize_;
  desc.lifeMin = lifeMin_;
  desc.lifeMax = lifeMax_;
  desc.color = color_;
  desc.baseScale = baseScale_;
  desc.scaleRandom = scaleRandom_;
  desc.baseRotate = baseRotate_;
  desc.rotateRandom = rotateRandom_;

  ParticleManager::GetInstance()->Emit(desc);
}

void ParticleEmitter::Emit(const Vector4 &overrideColor) {

  auto tmp = color_;
  color_ = overrideColor;
  Emit();
  color_ = tmp;
}