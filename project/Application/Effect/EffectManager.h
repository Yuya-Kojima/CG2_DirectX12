#pragma once
#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include <vector>
#include <memory>
#include <array>
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/BillboardParticleEmitter.h"

class RailCamera;
class ICamera;

class EffectManager {
public:
  static EffectManager *GetInstance();

  void Initialize();
  void Update(const ICamera* camera);
  void Draw();

  /// <summary>
  /// エディタ上のUI描画（テスト再生用カメラ情報を受け取る）
  /// </summary>
  void DrawEditorUI(RailCamera *railCamera);

  /// <summary>
  /// ショックウェーブ（波紋）を発生させる
  /// </summary>
  void PlayShockwave(const Vector3 &worldPos);

  /// <summary>
  /// 汎用的な敵の撃破エフェクト（爆発＋ショックウェーブ）を発生させる
  /// </summary>
  void PlayEnemyDeathEffect(const Vector3 &worldPos, const Vector4 &baseColor = {1.0f, 1.0f, 1.0f, 1.0f});

  /// <summary>
  /// ザコ敵用のシンプルな撃破エフェクト（コアのみ）を発生させる
  /// </summary>
  void PlayEnemyDeathSimpleEffect(const Vector3 &worldPos, const Vector4 &baseColor = {1.0f, 1.0f, 1.0f, 1.0f});

private:
  EffectManager() = default;
  ~EffectManager() = default;
  EffectManager(const EffectManager &) = delete;
  EffectManager &operator=(const EffectManager &) = delete;

  struct ShockwaveConfig {
    float duration = 0.5f;    // 再生時間
    float maxRadius = 0.8f;   // 最大半径
    float distortion = 0.05f; // 歪みの強さ
    float thickness = 0.1f;   // 波の太さ
  };
  ShockwaveConfig shockwaveConfig_;
  bool isShockwaveConfigDirty_ = false;

  struct ActiveShockwave {
    float timer;
    Vector3 worldPos;
  };
  std::vector<ActiveShockwave> activeShockwaves_;

  void SaveShockwaveConfig();
  void LoadShockwaveConfig();

  static const int kMaxHitEffects = 32;
  std::array<std::unique_ptr<BillboardParticleEmitter>, kMaxHitEffects> hitCoreParticleGroups_;
  std::array<std::unique_ptr<BillboardParticleEmitter>, kMaxHitEffects> hitFlareParticleGroups_;
  std::array<std::unique_ptr<BillboardParticleEmitter>, kMaxHitEffects> hitRingParticleGroups_;

  std::array<std::unique_ptr<ParticleEmitter>, kMaxHitEffects> deathCoreEmitters_;
  std::array<std::unique_ptr<ParticleEmitter>, kMaxHitEffects> deathFlareEmitters_;
  std::array<std::unique_ptr<ParticleEmitter>, kMaxHitEffects> deathRingEmitters_;
  int nextHitEffectIndex_ = 0;
};
