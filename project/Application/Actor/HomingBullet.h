#pragma once
#include "Framework/BaseActor.h"
#include "Math/Vector3.h"
#include <memory>

class Object3d;
class Object3dRenderer;
class Enemy;

class HomingBullet : public BaseActor {
public:
  HomingBullet();
  ~HomingBullet() override;

  /// <summary>
  /// ホーミング弾の初期化
  /// </summary>
  /// <param name="renderer">3D描画用レンダラー</param>
  /// <param name="startPos">発射開始座標（自機の位置）</param>
  /// <param name="target">追従する対象</param>
  /// <param name="initialVelocity">発射直後の初速ベクトル（散らばり用）</param>
  void Initialize(Object3dRenderer* renderer, const Vector3& startPos, Enemy* target, const Vector3& initialVelocity);
  
  // アクションエディタから受け取ったパラメータをセットする
  void SetHomingParams(float speed, int fallTime, float strengthIncrease, float strengthMax) {
    speed_ = speed;
    homingFallTime_ = fallTime;
    homingStrengthIncrease_ = strengthIncrease;
    homingStrengthMax_ = strengthMax;
  }

  void Update() override;
  void Draw3D() override;

  bool IsDead() const { return isDead_; }

private:
  std::unique_ptr<Object3d> object3d_;
  Enemy* target_ = nullptr;
  
  Vector3 velocity_;
  float speed_ = 1.5f;           // 弾の飛ぶ速さ
  float homingStrength_ = 0.02f; // 誘導の強さ（小さいほど大回りする）
  int lifeTimer_ = 180;          // 寿命（約3秒で消滅）
  
  // アクションエディタ用パラメータ（初期値は元コードと同じ）
  int homingFallTime_ = 165;
  float homingStrengthIncrease_ = 0.015f;
  float homingStrengthMax_ = 0.25f;

  bool isDead_ = false;
};
