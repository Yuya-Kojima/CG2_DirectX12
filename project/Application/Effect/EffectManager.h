#pragma once
#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include <vector>

class RailCamera;

class EffectManager {
public:
  static EffectManager *GetInstance();

  void Initialize();
  void Update(const Matrix4x4 &viewProj);

  /// <summary>
  /// エディタ上のUI描画（テスト再生用カメラ情報を受け取る）
  /// </summary>
  void DrawEditorUI(RailCamera *railCamera);

  /// <summary>
  /// ショックウェーブ（波紋）を発生させる
  /// </summary>
  void PlayShockwave(const Vector3 &worldPos);

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
};
