#pragma once

#include "Math/MathUtil.h"
#include <string>

class ParticleEmitter {

public:
  /// <summary>
  ///
  /// </summary>
  /// <param name="name">パーティクルグループ名</param>
  /// <param name="center">エミッター中心座標</param>
  /// <param name="halfSize">エミッターのサイズ/2</param>
  /// <param name="count">一度に生成する数</param>
  /// <param name="frequency">生成間隔</param>
  /// <param name="baseVel">パーティクルの基本速度</param>
  /// <param name="velRandom">速度に加える乱数(バラつかせ)</param>
  /// <param name="lifeMin">最短寿命</param>
  /// <param name="lifeMax">最長寿命</param>
  ParticleEmitter(const std::string &name, Vector3 center, Vector3 halfSize,
                  int count, float frequency, Vector3 baseVel,
                  Vector3 velRandom, float lifeMin, float lifeMax);

  /// <summary>
  /// 生成間隔(frequency)に応じてパーティクル生成
  /// </summary>
  void Update();

  /// <summary>
  /// パーティクルを手動で生成
  /// </summary>
  void Emit();

  /// <summary>
  /// 色を指定して生成
  /// </summary>
  /// <param name="overrideColor">生成するパーティクルの色</param>
  void Emit(const Vector4 &overrideColor);

  void SetCenter(const Vector3 &center) { center_ = center; }

  void SetHalfSize(const Vector3 &halfSize);

  void SetCount(int count);
  void SetFrequency(float frequency);

  void SetBaseVelocity(const Vector3 &v) { baseVelocity_ = v; }
  void SetVelocityRandom(const Vector3 &v) { velocityRand_ = v; }

  void SetLifeRange(float lifeMin, float lifeMax);

  void SetColor(const Vector4 &color) { color_ = color; }

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
