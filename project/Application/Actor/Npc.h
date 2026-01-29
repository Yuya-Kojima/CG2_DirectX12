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

// ============================================================
// NPC（自律移動キャラクター）クラス
// 経路探索・到達行動・衝突処理などを内包するAI制御オブジェクト
// ============================================================
class Npc {
public:
    // 初期化
    void Initialize(Object3dRenderer* renderer, const Vector3& startPos);

    // 毎フレーム更新
    // dt : 経過時間
    // targetPos : 目標地点
    // level : ステージ情報
    void Update(float dt, const Vector3& targetPos, Level& level);

    // 描画処理
    void Draw();

    // ===== アクセサ =====
    const Vector3& GetPosition() const { return pos_; } // 現在位置取得
    void SetNavGrid(const NavigationGrid* nav) { nav_ = nav; } // ナビグリッド設定
    void SetLayer(uint32_t v) { layer_ = v; } // レイヤー設定
    uint32_t GetLayer() const { return layer_; } // レイヤー取得
    void SetId(uint32_t id) { id_ = id; } // ID設定
    uint32_t GetId() const { return id_; } // ID取得

    // ステージ側プロパティから行動パターンを設定（例: followPlayer など）
    void SetBehavior(const std::string& b) { behavior_ = b; }
    const std::string& GetBehavior() const { return behavior_; }

    // レベル参照を保持（追加クエリや衝突処理で使用）
    void AttachLevel(class Level* level) { level_ = level; }

private:
    Object3dRenderer* renderer_ = nullptr; // 描画レンダラー
    std::unique_ptr<Object3d> obj_; // 描画用3Dオブジェクト本体

    
    // 定数
    // NPC の当たり判定半径のデフォルト値
    static constexpr float kDefaultRadius = 0.3f;
    // 再経路探索を行う間隔（秒）
    static constexpr float kReplanInterval = 0.5f;
    // 距離ゼロ判定のための小さな閾値
    static constexpr float kSmallEpsilon = 0.0001f;
    // 経路中継点に到達したとみなす閾値
    static constexpr float kWaypointThreshold = 0.3f;
    // 接地判定で使用するレイの高さ（段差許容値）
    static constexpr float kStepHeight = 0.3f;
    // 接地判定の追加余裕
    static constexpr float kStepExtra = 0.2f;


    // ===== 移動関連パラメータ =====
    Vector3 pos_ { 0, 0, 0 }; // 現在位置
    Vector3 vel_ { 0, 0, 0 }; // 現在速度
    float moveSpeed_ = 3.5f; // 最大移動速度
    float accel_ = 15.0f; // 加速度
    float decel_ = 20.0f; // 減速度
    float desiredDistance_ = 1.5f; // 目標地点に到達とみなす距離
    float slowRadius_ = 3.0f; // 減速開始半径

    // ===== 経路探索関連 =====
    const NavigationGrid* nav_ = nullptr; // ナビゲーショングリッド参照
    std::vector<Vector3> path_; // A*などで算出された経路点
    int pathIndex_ = 0; // 現在目指している経路点インデックス
    float replanTimer_ = 0.0f; // 再経路探索タイマー

    // 最後に経路探索を行ったときのタイル座標を保持し、同じなら再計画をスキップする
    // 初期値は -1（未設定）
    int lastSearchStartX_ = -1;
    int lastSearchStartZ_ = -1;
    int lastSearchGoalX_ = -1;
    int lastSearchGoalZ_ = -1;

    Level* level_ = nullptr; // レベル参照

    uint32_t layer_ = 0; // 所属レイヤー
    uint32_t id_ = 0; // 固有ID
    std::string behavior_; // 行動タイプ識別子
};