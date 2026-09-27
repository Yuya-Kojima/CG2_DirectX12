#pragma once
#include "IEnemyBehavior.h"
#include "Math/Vector3.h"

/// <summary>
/// 固定砲台ビヘイビア（建造物・壁面に設置され、プレイヤーを感知・先読み迎撃するAI）
/// 遠方フォグ内では休眠（Dormant）し、プレイヤー接近を検知すると起動（Active）して照準・迎撃を行う。
/// </summary>
class BehaviorTurret : public IEnemyBehavior {
public:
  struct Config {
    float senseDistance = 250.0f;  // 感知距離（m）：この距離に入ると起動して照準開始
    float fireDelay = 1.0f;        // 起動から初弾発射までの待機・照準時間（秒）
    float fireInterval = 2.0f;     // 次弾発射間隔（秒）
    float bulletSpeed = 1.3f;      // 弾速（フレームあたり移動距離）
    float minFairDistance = 45.0f; // すれ違い直前の撃ちやめ距離（理不尽な真横・背後撃ち防止）
  };

  BehaviorTurret();
  explicit BehaviorTurret(const Config& config);
  ~BehaviorTurret() override = default;

  void Update(Enemy* enemy) override;
  bool IsLockOnTarget() const override { return state_ != State::Dormant; }

  void SetConfig(const Config& config) { config_ = config; }
  const Config& GetConfig() const { return config_; }

private:
  enum class State {
    Dormant, // 休眠待機（遠方フォグ内：初期向きを向いて沈黙）
    Active,  // 起動・照準（感知距離内：自機をエイム追尾）
    Combat   // 迎撃戦闘（射撃中）
  };

  Config config_;
  State state_ = State::Dormant;
  float activeTimer_ = 0.0f;
  float shotTimer_ = 0.0f;
  Vector3 prevPlayerPos_ = {0.0f, 0.0f, 0.0f};
  bool hasPrevPlayerPos_ = false;
  bool isInitialRotationSet_ = false;
  Vector3 restRotation_ = {0.0f, 0.0f, 0.0f};
};
