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

    // ===== 乗車状態情報（ログ表示用） =====
    bool IsMounted() const { return mounted_; }
    uint32_t GetMountedOwnerId() const { return mountedOwnerId_; }
    // ===== 状態制御（State machine） =====
    // NPCの行動状態を簡易的なStateで管理します
    // 現在は単純な直進モードとアイドルのみを保持します。
    enum class State {
        Idle,
        Straight      // 与えられた方向へ直進するモード
    };

    // 状態の設定
    void SetState(State s) { state_ = s; }

    // 直進モードの方向ベクトルを設定（XZ成分を使用）
    void SetStraightDirection(const Vector3& dir) { straightDir_ = dir; }

    State GetState() const { return state_; }
    // もう少し上げて見た目の干渉を減らす
    // NPCがオブジェクト上に乗せられたときのYオフセット（モデル原点と足元の差を補正）
    // もう少し上げて見た目の干渉を減らす
    // デフォルト値を若干増やして、メッシュのサイズ変更で低く見える場合に備える
    float mountOffsetY_ = 0.40f;

public:
    // マウント時のYオフセットを設定（単位: ワールド座標）
    void SetMountYOffset(float v) { mountOffsetY_ = v; }
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

    // マウント継続時間（秒）
    static constexpr float kMountDuration = 0.8f;

    // mount entering: short smoothing period when NPC first lands on object
    static constexpr float kMountEnterTime = 0.18f; // seconds
    float mountEnterTimer_ = 0.0f;
    bool mountEntering_ = false;
    // mount interpolation targets
    Vector3 mountStartPos_ {0.0f,0.0f,0.0f};
    Vector3 mountTargetPos_ {0.0f,0.0f,0.0f};
    float mountStartYaw_ = 0.0f;
    float mountTargetYaw_ = 0.0f;

    // Contact slowdown: when NPC is repeatedly steering or pushing against obstacles,
    // temporarily reduce forward speed to avoid 'boost' through thin obstacles.
    static constexpr float kContactSlowTime = 0.25f; // seconds
    // make slowdown less severe so NPC doesn't come to a full stop
    static constexpr float kContactSlowFactor = 0.9f; // multiplier to moveSpeed_
    float contactTimer_ = 0.0f;


    // ===== 移動関連パラメータ =====
    Vector3 pos_ { 0, 0, 0 }; // 現在位置
    Vector3 vel_ { 0, 0, 0 }; // 現在速度
    float moveSpeed_ = 2.7f; // 最大移動速度（少し増加）
    float accel_ = 12.0f; // 加速度（少し増やす）
    float decel_ = 14.0f; // 減速度
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

    // ===== State / Straight movement =====
    // デフォルトは直進モードにする（経路探索を使わず単純移動させる）
    State state_ = State::Straight; // 現在の状態
    Vector3 straightDir_ { 0.0f, 0.0f, 1.0f }; // 直進モード時の方向
    // 乗車状態フラグ（OBBに乗っているか）および該当OBBのownerId
    bool mounted_ = false;
    uint32_t mountedOwnerId_ = 0;
    // マウント経過時間（秒）
    float mountTimer_ = 0.0f;
    // マウント解除後に地面へ落下するフラグ
    bool fallingFromMount_ = false;
    // 降下後に一時的に前進を止めるクールダウン
    static constexpr float kPostFallCooldown = 0.15f;
    float postFallCooldown_ = 0.0f;
    // grace time before unmounting when contact is briefly lost (seconds)
    // increased to be more tolerant of short contact blips
    static constexpr float kMountGrace = 0.25f;
    // マウントしたOBBのパラメータを保持（アンマウント判定に使用）
    Vector3 mountedObbCenter_ { 0.0f, 0.0f, 0.0f };
    Vector3 mountedObbHalfExtents_ { 0.0f, 0.0f, 0.0f };
    float mountedObbYaw_ = 0.0f;
    // ===== 配達物（表示用） =====
    std::unique_ptr<class Object3d> packageObj_; // 持ち物モデル
    bool carrying_ = false; // 最初は持っていないが外から指定できる
    Vector3 packageOffset_ = { 0.0f, 0.0f, 0.0f }; // NPC基準のオフセット
public:
    // 配達物表示の制御
    void SetCarrying(bool v) { carrying_ = v; }
    bool IsCarrying() const { return carrying_; }
    
    // ===== 配達 / 帰還フラグ =====
    // スポーン地点（戻る場所）
    Vector3 spawnPos_ { 0.0f, 0.0f, 0.0f };
    // NPCが配達後にスポーンへ戻っているかどうか
    bool returning_ = false;

    // Smooth facing control
    float targetYaw_ = 0.0f;
    float yawSmoothSpeed_ = 4.5f; // radians per second（やや速め）

    // 配達完了・帰還制御
    void BeginReturnToSpawn();
    bool IsReturning() const { return returning_; }
    bool HasReturnedToSpawn() const;
    
    // target yaw for smooth rotation and smoothing speed (radians/sec)
    void SetYawSmoothSpeed(float s) { yawSmoothSpeed_ = s; }
};