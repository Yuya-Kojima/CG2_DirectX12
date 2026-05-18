#pragma once
#include "Math/Matrix4x4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Render/Sprite/Sprite.h"
#include <memory>
#include <vector>
#include <array>

class Object3d;
class SpriteRenderer;

/// <summary>
/// ロックオンを管理する専用クラス
/// </summary>
class LockOn {
public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(SpriteRenderer *spriteRenderer);

  /// <summary>
  /// 毎フレームの更新
  /// </summary>
  /// <param name="enemies">ターゲット候補となる敵のリスト</param>
  /// <param name="viewProjectionMatrix">現在のカメラのViewProjection行列</param>
  /// <param name="reticlePos">プレイヤーのメイン照準の2D座標</param>
  /// <param name="isLockOnMode">プレイヤーがロックオンボタンを長押ししているか</param>
  void Update(const std::vector<Object3d *> &enemies,
              const Matrix4x4 &viewProjectionMatrix,
              const Vector2 &reticlePos,
              bool isLockOnMode);

  /// <summary>
  /// 2Dカーソルの描画
  /// </summary>
  void Draw();

  /// <summary>
  /// ロックオン状態を解除
  /// </summary>
  void Clear() { targets_.clear(); }

  const std::vector<Object3d*>& GetTargets() const { return targets_; }

private:
  static const size_t kMaxLockOnCount = 8; // 最大ロックオン数
  std::vector<Object3d *> targets_; // ロックオン中の敵リスト
  
  // 照準（マーカー）用スプライトを最大ロックオン数分用意
  std::array<std::unique_ptr<Sprite>, kMaxLockOnCount> reticles_;
  
  Matrix4x4 viewProjectionMatrix_;  // 描画用のカメラ行列キャッシュ
};
