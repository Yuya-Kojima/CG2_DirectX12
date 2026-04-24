#pragma once

#include "Math/MathUtil.h"
#include <string>

class IParticleEmitter;

class ParticleEmitter {

public:
  /// <summary>
  /// パーティクルを自動発生させるスポナーを初期化します
  /// </summary>
  /// <param name="targetEmitter">パーティクルを描画する対象の実体（グループ）</param>
  /// <param name="center">エミッターの中心座標</param>
  /// <param name="halfSize">発生範囲の広さ（AABBの半分のサイズ。0なら一点から発生）</param>
  /// <param name="count">一度のタイミングで発生させるパーティクルの数</param>
  /// <param name="frequency">発生間隔（秒）。例: 1.0f なら1秒ごとに発生</param>
  /// <param name="baseVel">パーティクルの基本となる初速度</param>
  /// <param name="velRandom">初速度に加えるランダムなばらつき（±）</param>
  /// <param name="lifeMin">パーティクルの最短寿命（秒）</param>
  /// <param name="lifeMax">パーティクルの最長寿命（秒）</param>
  ParticleEmitter(IParticleEmitter* targetEmitter, Vector3 center, Vector3 halfSize,
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

  void SetBaseScale(const Vector3 &s) { baseScale_ = s; }
  void SetScaleRandom(const Vector3 &s) { scaleRandom_ = s; }

  void SetBaseRotate(const Vector3 &r) { baseRotate_ = r; }
  void SetRotateRandom(const Vector3 &r) { rotateRandom_ = r; }

private:
  IParticleEmitter* targetEmitter_ = nullptr;
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
  Vector3 baseScale_{1.0f, 1.0f, 1.0f};
  Vector3 scaleRandom_{};
  Vector3 baseRotate_{};
  Vector3 rotateRandom_{};
};
