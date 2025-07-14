#pragma once
#include <random>

struct Particle {
  Vector2 pos;
  Vector2 vel;
  float age;
  float life;
};

struct ParticleForGPU {
  Matrix4x4 WVP;
  Matrix4x4 World;
  Vector4 color;
};

std::vector<Particle> particles;
std::mt19937 rng{std::random_device{}()};

void UpdateParticles(float dt) {
  // まず既存を更新
  for (auto &p : particles) {
    p.age += dt;
    p.pos += p.vel * dt;
  }
  // 寿命切れを消す
  particles.erase(std::remove_if(particles.begin(), particles.end(),
                                 [](auto const &p) { return p.age >= p.life; }),
                  particles.end());
}

void EmitParticles(int count) {
  std::uniform_real_distribution<float> ud(0.0f, 1.0f);
  std::uniform_real_distribution<float> lifeDist(1.0f, 2.0f);
  for (int i = 0; i < count; ++i) {
    Particle p;
    p.pos = {900.0f + (ud(rng) - 0.5f) * 20.0f,
             500.0f}; // 右下文字位置の少しばらつき
    float ang =
        3.1415f * (0.5f + (ud(rng) - 0.5f) * 0.2f); // 上向き＋少しノイズ
    float speed = 50.0f + ud(rng) * 50.0f;
    p.vel = {cosf(ang) * speed, sinf(ang) * speed};
    p.age = 0.0f;
    p.life = lifeDist(rng);
    particles.push_back(p);
  }
}