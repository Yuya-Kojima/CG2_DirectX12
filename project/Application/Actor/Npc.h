#pragma once
#include "Math/Vector3.h"
#include "Navigation/NavigationGrid.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Object3d;
class Object3dRenderer;
class Level;

class Npc {
public:
    void Initialize(Object3dRenderer* renderer, const Vector3& startPos);
    void Update(float dt, const Vector3& targetPos, Level& level);
    void Draw();

    // アクセサ
    const Vector3& GetPosition() const { return pos_; }
    void SetNavGrid(const NavigationGrid* nav) { nav_ = nav; }
    void SetLayer(uint32_t v) { layer_ = v; }
    uint32_t GetLayer() const { return layer_; }
    void SetId(uint32_t id) { id_ = id; }
    uint32_t GetId() const { return id_; }

    // ステージプロパティからの振る舞い設定 (例: "gotoGoal", "followPlayer")
    void SetBehavior(const std::string& b) { behavior_ = b; }
    const std::string& GetBehavior() const { return behavior_; }

private:
    Object3dRenderer* renderer_ = nullptr;
    std::unique_ptr<Object3d> obj_;

    // 移動パラメータ
    Vector3 pos_ { 0, 0, 0 }; // 現在座標
    Vector3 vel_ { 0, 0, 0 }; // 現在速度
    float moveSpeed_ = 3.5f; // 最大移動速度
    float accel_ = 15.0f; // 加速度
    float decel_ = 20.0f; // 減速度
    float desiredDistance_ = 1.5f; // 停止距離
    float slowRadius_ = 3.0f; // 減速を開始する半径

    // 経路探索・ナビゲーション
    const NavigationGrid* nav_ = nullptr; // グリッド情報への参照
    std::vector<Vector3> path_; // 算出された経路点リスト
    int pathIndex_ = 0; // 現在目指している経路のインデックス
    float replanTimer_ = 0.0f; // 再経路探索までのタイマー

    uint32_t layer_ = 0; // 所属レイヤー
    uint32_t id_ = 0; // 個別ID
    std::string behavior_; // 振る舞い識別子
};