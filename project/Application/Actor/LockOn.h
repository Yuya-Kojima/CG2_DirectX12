#pragma once
#include "Math/Matrix4x4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Sprite/Sprite.h"
#include <memory>
#include <vector>

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
  /// <param
  /// name="viewProjectionMatrix">現在のカメラのViewProjection行列</param>
  void Update(const std::vector<Object3d *> &enemies,
              const Matrix4x4 &viewProjectionMatrix);

  /// <summary>
  /// 2Dカーソルの描画
  /// </summary>
  void Draw();

  /// <summary>
  /// 現在ロックオン中のターゲットを取得
  /// </summary>
  Object3d *GetTarget() const { return target_; }

private:
  std::unique_ptr<Sprite> reticle_; // ロックオンカーソルのスプライト
  Object3d *target_ = nullptr;      // 現在ロックオンしている敵
};
