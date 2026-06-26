#pragma once
#include "IEnemyBehavior.h"
#include "Math/Vector3.h"

// プレイヤーを注視し、一定時間で回避行動を取る自律型戦闘機AI
class BehaviorFighter : public IEnemyBehavior {
public:
    BehaviorFighter();
    void Update(Enemy* enemy) override;

private:
    enum class State {
        Enter,   // 登場（定位置につく）
        Combat,  // 戦闘（プレイヤーを注視しながら漂う）
        Evade,   // 回避（急加速して避ける）
        Retreat  // 離脱（画面外へ飛び去る）
    };

    State state_ = State::Enter;
    float stateTimer_ = 0.0f;
    Vector3 evadeDir_ = {0.0f, 0.0f, 0.0f};

    void UpdateEnter(Enemy* enemy);
    void UpdateCombat(Enemy* enemy);
    void UpdateEvade(Enemy* enemy);
    void UpdateRetreat(Enemy* enemy);
};
