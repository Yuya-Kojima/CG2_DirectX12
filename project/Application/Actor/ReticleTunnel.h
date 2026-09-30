#pragma once
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include "Render/Sprite/Sprite.h"
#include <memory>
#include <array>

class SpriteRenderer;
class ICamera;
class Input;

/// <summary>
/// パンツァードラグーン風 3D視線トンネル照準（三重矩形）クラス
/// </summary>
class ReticleTunnel {
public:
  struct Config {
    // 2D照準移動パラメータ（操作感）
    float reticleMaxSpeed = 25.0f;     // 照準の最高移動速度

    // 3D視線トンネルパラメータ（奥行き・サイズ）
    float nearDistance = 35.0f;        // 手前枠の3D奥行き距離
    float midDistance = 70.0f;         // 中間枠の3D奥行き距離
    float farDistance = 120.0f;        // 奥枠の3D奥行き距離

    float nearSize = 3.5f;             // 手前枠の3D基本サイズ（ロックオン範囲に合わせて拡大）
    float midSize = 2.6f;              // 中間枠の3D基本サイズ
    float farSize = 2.0f;              // 奥枠の3D基本サイズ

    float nearRotSpeed = 0.4f;         // 手前枠の回転スピード（ラジアン/秒）
    float midRotSpeed = -0.25f;        // 中間枠の回転スピード
    float farRotSpeed = 0.6f;          // 奥枠の回転スピード

    float lineThickness = 2.5f;        // 枠線の太さ（ピクセル）
    float targetDistance = 1000.0f;    // 射撃目標までの基本距離
  };

  ReticleTunnel() = default;
  ~ReticleTunnel() = default;

  /// <summary>
  /// 初期化処理
  /// </summary>
  void Initialize(SpriteRenderer* spriteRenderer);

  /// <summary>
  /// 毎フレームの更新処理（入力操作・3D視線計算・頂点投影）
  /// </summary>
  void Update(Input* input, const ICamera* camera, const Vector3& playerPos, bool isLockOn);

  /// <summary>
  /// 2Dトンネル枠線スプライトの描画
  /// </summary>
  void Draw();

  /// <summary>
  /// 照準座標を画面中央にリセット
  /// </summary>
  void ResetPosition();

  // プレイヤーおよび武器システム用のアクセサ
  const Vector2& Get2DPosition() const { return position_; }
  const Vector3& GetTarget3DPosition() const { return target3DPos_; }
  const Vector3& GetRayDirection() const { return rayDirection_; }

  Config& GetConfig() { return config_; }

private:
  Config config_;
  SpriteRenderer* spriteRenderer_ = nullptr;

  float lockOnLerp_ = 0.0f; // ロックオン状態のアニメーション用（0.0〜1.0）

  Vector2 position_ = { 640.0f, 360.0f }; // 2Dスクリーン座標（中央 1280x720 想定）

  Vector3 target3DPos_ = { 0.0f, 0.0f, 0.0f };  // 3D空間上の射撃目標地点
  Vector3 rayDirection_ = { 0.0f, 0.0f, 1.0f }; // 3D視線の単位方向ベクトル

  float nearRot_ = 0.0f; // 手前枠の累積回転角
  float midRot_ = 0.0f;  // 中間枠の累積回転角
  float farRot_ = 0.0f;  // 奥枠の累積回転角

  // トンネル構造を描画するための直線スプライト群
  // 手前(4本) + 中(4本) + 奥(4本) ＝ 12本
  // 手前-中-奥の角を結ぶトンネル接続線 ＝ 8本 (Near->Mid 4本 + Mid->Far 4本)
  // 合計 20本
  // 枠線(手前・中・奥) × 4本 × 2重(アウトライン+本体) = 24本
  static constexpr size_t kMaxLineSprites = 24;
  std::array<std::unique_ptr<Sprite>, kMaxLineSprites> lineSprites_;

  // スプライトの更新ヘルパー
  void UpdateLineSprite(size_t index, const Vector2& p1, const Vector2& p2, float thickness, const Vector4& color);
};
