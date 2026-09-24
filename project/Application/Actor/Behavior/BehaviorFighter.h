#pragma once
#include "IEnemyBehavior.h"
#include "Math/Vector3.h"

/// <summary>
/// 画面外からスムーズに進入し、自機前方でホバリング滞空してプレイヤーと対峙、
/// 一定時間後に回避・離脱行動を取る汎用自律戦闘機AI。
/// 各フェーズの時間や浮遊の振幅・周波数はConfig構造体を通じて外部から柔軟にカスタマイズ可能。
/// </summary>
class BehaviorFighter : public IEnemyBehavior {
public:
    /// <summary>
    /// 汎用自律戦闘機AIの動作パラメータ
    /// </summary>
    struct Config {
        float enterDuration = 0.9f;        // 画面外から定位置までの進入時間（秒）
        float combatDuration = 4.2f;       // 自機前方でホバリング滞空する時間（秒）
        float evadeDuration = 1.0f;        // 回避行動の時間（秒）
        float hoverAmplitude = 1.6f;       // ホバリングの上下浮遊の振幅（m）
        float hoverFrequency = 2.0f;       // ホバリングの浮遊周波数（rad/s）
        float rollAmplitude = 0.12f;       // 生き物・飛行感を表すロール角の揺らぎ（rad）
        float enterOffsetDistance = 75.0f; // 画面外の出現オフセット距離（左右方向）
    };

    BehaviorFighter();
    explicit BehaviorFighter(const Config& config);
    void Update(Enemy* enemy) override;

    void SetConfig(const Config& config) { config_ = config; }
    const Config& GetConfig() const { return config_; }

private:
    enum class State {
        Enter,   // 登場（画面外からイージングで定位置に滑り込む）
        Combat,  // 戦闘滞空（自機と速度同期し、上下にホバリングしながら注視）
        Evade,   // 回避（左右へロールしながら急加速して離脱開始）
        Retreat  // 離脱（画面外へ飛び去り、安全にデスポーン）
    };

    State state_ = State::Enter;
    float stateTimer_ = 0.0f;
    Vector3 evadeDir_ = {0.0f, 0.0f, 0.0f};
    Vector3 startOffset_ = {0.0f, 0.0f, 0.0f};
    bool isEnterInitialized_ = false;

    Config config_;

    void UpdateEnter(Enemy* enemy, const Vector3& basePos, const Vector3& baseRight, const Vector3& baseUp, const Vector3& baseForward);
    void UpdateCombat(Enemy* enemy, const Vector3& basePos, const Vector3& baseRight, const Vector3& baseUp, const Vector3& baseForward);
    void UpdateEvade(Enemy* enemy, const Vector3& basePos, const Vector3& baseRight, const Vector3& baseUp, const Vector3& baseForward);
    void UpdateRetreat(Enemy* enemy, const Vector3& basePos, const Vector3& baseRight, const Vector3& baseUp, const Vector3& baseForward);
};
