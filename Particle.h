#pragma once
#include "Transform.h"
#include "Vector3.h"
#include <random>

struct Particle {
  Transform transform;
  Vector3 velocity;
};

Particle MakeNewParticle(std::mt19937 &randomEngine) {

  std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

  Particle particle;
  particle.transform.scale = {1.0f, 1.0f, 1.0f};
  particle.transform.rotate = {0.0f, 0.0f, 0.0f};
  particle.transform.translate = {distribution(randomEngine),
                                  distribution(randomEngine),
                                  distribution(randomEngine)};
  particle.velocity = {distribution(randomEngine), distribution(randomEngine),
                       distribution(randomEngine)};

  return particle;
}