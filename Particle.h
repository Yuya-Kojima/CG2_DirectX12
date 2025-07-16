#pragma once
#include <numbers>
#include <random>

struct Particle {
  Vector2 pos;
  Vector2 vel;
  float currentTime;
  float lifeTime;
};

struct ParticleForGPU {
  Matrix4x4 WVP;
  Matrix4x4 World;
  Vector4 color;
};

std::vector<Particle> particles;
std::mt19937 randomEngine{std::random_device{}()};

void UpdateParticles(float deltaTime) {
  for (auto &particle : particles) {
    particle.currentTime += deltaTime;
    particle.pos += particle.vel * deltaTime;
  }

  // 寿命切れを消す
  std::erase_if(particles,
                [](auto const &p) { return p.currentTime >= p.lifeTime; });
}

/// <summary>
/// パーティクル生成
/// </summary>
/// <param name="count">一回で生成する数</param>
/// <param name="origin">発生座標</param>
void EmitParticles(int count, Vector2 origin, const int &kNumMaxInstance) {

  std::uniform_real_distribution<float> distAngle(-0.2f, 0.2f);
  std::uniform_real_distribution<float> distOffset(-10.0f, 10.0f);
  std::uniform_real_distribution<float> distSpeed(-20.0f, 20.0f);
  std::uniform_real_distribution<float> distTime(0.3f, 2.0f);

  for (int i = 0; i < count && particles.size() < kNumMaxInstance; ++i) {
    Particle particle;
    // 発生位置にも少し乱数を＋する
    particle.pos = {origin.x + distOffset(randomEngine), origin.y};

    // 上向き+ランダム
    float angle = -std::numbers::pi_v<float> / 2.0f + distAngle(randomEngine);

    // 基本速度+乱数 少しだけ速度をバラつかせる
    float speed = 100.0f + distSpeed(randomEngine) * 2.0f;

    particle.vel.x = cosf(angle) * speed;
    particle.vel.y = sinf(angle) * speed;

    particle.currentTime = 0.0f;
    particle.lifeTime = distTime(randomEngine);
    particles.push_back(particle);
  }
}