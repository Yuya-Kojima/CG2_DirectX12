#pragma once
#include "Transform.h"
#include "Vector3.h"
#include <random>

struct Particle {
  Transform transform;
  Vector3 velocity;
  Vector4 color;
  float lifeTime;
  float currentTime;
};

Particle MakeNewParticle(std::mt19937 &randomEngine) {

  std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

  Particle particle;
  particle.transform.scale = {1.0f, 1.0f, 1.0f};

  particle.transform.rotate = {0.0f, 0.0f, 0.0f};

  particle.transform.translate = {
      distribution(randomEngine),
      distribution(randomEngine),
      distribution(randomEngine),
  };

  particle.velocity = {
      distribution(randomEngine),
      distribution(randomEngine),
      distribution(randomEngine),
  };

  particle.color = {
      distribution(randomEngine),
      distribution(randomEngine),
      distribution(randomEngine),
      1.0f,
  };

  std::uniform_real_distribution<float> distTime(1.0f,3.0f);
  particle.lifeTime = distTime(randomEngine);
  particle.currentTime = 0;

  return particle;
}