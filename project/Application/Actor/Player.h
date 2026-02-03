#pragma once
#include "Camera/GameCamera.h"
#include "Math/Vector3.h"
#include <cstdint>

class Object3d;
class Object3dRenderer;
#include "Actor/Level.h"
#include "Input/Input.h"

/// <summary>
/// プレイヤーキャラクターを制御するクラス
/// 入力移動、ジャンプ、重力、衝突判定、カメラ追従を管理します
/// </summary>
class Player {
public:
    Player();
    ~Player();

    // --- 初期化・基本機能 ---

    /// <summary>
    /// エンジンの入力管理とレンダラを受け取って初期化します
    /// </summary>
    void Initialize(Input* input, Object3dRenderer* object3dRenderer);

    /// <summary>
    /// 毎フレームの更新処理（移動計算、衝突解決、カメラ更新など）
    /// </summary>
    void Update(float deltaTime);

    /// <summary>
    /// 3Dモデルの描画
    /// </summary>
    void Draw();

    // --- 手(腕)のポーズ制御 ---
    /// <summary> 棒を持っているときの手の配置を設定（シーンから呼び出す） </summary>
    void SetHandPoseHeld(bool sideRight);

    /// <summary> 棒を持っていないときのリラックスポーズ（下ろした状態） </summary>
    void SetHandPoseDropped();

    // --- 座標・トランスフォーム関連 ---

    /// <summary> 現在のワールド座標を取得 </summary>
    const Vector3& GetPosition() const { return position_; }

    /// <summary> ワールド座標を直接設定（描画オブジェクトにも即座に反映） </summary>
    void SetPosition(const Vector3& pos);

    /// <summary> 衝突判定に使う半幅（XZ平面）を取得 </summary>
    float GetHalfSize() const { return halfSize_; }
    /// <summary> 衝突判定に使う半幅ベクトル（XZ平面）を取得 </summary>
    Vector3 GetHalfExtents() const { return Vector3{ halfSize_, 0.0f, halfSize_ }; }

    // --- カメラ制御関連 ---

    /// <summary> プレイヤーを追跡するカメラを登録 </summary>
    void AttachCamera(GameCamera* camera) { camera_ = camera; }

    /// <summary> カメラアタッチ時のスムージング時間を設定 </summary>
    void SetAttachSmoothingDuration(float d) { attachDuration_ = d; }

    /// <summary> カメラのオフセット（プレイヤーからの距離）を設定 </summary>
    void SetCameraFollowOffset(const Vector3& off) { followOffset_ = off; }

    /// <summary> カメラ追従の滑らかさを設定 </summary>
    void SetCameraFollowSmooth(float s) { followSmooth_ = s; }

    /// <summary> カメラを即座に特定の位置から追従開始（補間あり）させる </summary>
    void AttachCameraImmediate(GameCamera* cam);

    // --- レベル・衝突判定設定 ---

    /// <summary> 衝突判定の対象となるステージ情報を登録 </summary>
    void AttachLevel(class Level* level);

    /// <summary> 衝突レイヤーの設定 </summary>
    void SetLayer(uint32_t v) { layer_ = v; }
    uint32_t GetLayer() const { return layer_; }

    /// <summary> 個別識別IDの設定 </summary>
    void SetId(uint32_t id) { id_ = id; }
    uint32_t GetId() const { return id_; }

private:
    // --- 内部参照・コンポーネント ---
    GameCamera* camera_ = nullptr; // 使用中のカメラ
    Input* input_ = nullptr; // キーボード/パッド入力
    Object3d* obj_ = nullptr; // 描画用モデル実体
    Object3d* objHand_ = nullptr; // 手用モデル（任意）

    // --- 動力学パラメータ ---
    Vector3 position_ { 0.0f, 0.0f, 0.0f }; // 現在地
    Vector3 velocity_ { 0.0f, 0.0f, 0.0f }; // 現在速度ベクトル
    float rotate_ = 0.0f; // 向き（ラジアン）

    // --- 移動定数（調整用） ---
    float moveSpeed_ = 6.0f; // 最高速度
    float accel_ = 20.0f; // 加速度（0から最高速まで）
    float decel_ = 25.0f; // 減速度（入力なし時の止まりやすさ）
    float gravity_ = 30.0f; // 下向きの重力
    float jumpSpeed_ = 12.0f; // ジャンプした瞬間の上向き速度
    bool onGround_ = true; // 接地状態フラグ
    float halfSize_ = 0.5f; // キャラクターの当たり判定半径

    // --- レベル・ID情報 ---
    Level* level_ = nullptr; // 地形情報への参照
    uint32_t layer_ = 0; // 所属レイヤー
    uint32_t id_ = 0; // 固有ID

    // --- カメラ追従用内部変数 ---
    Vector3 followOffset_ { -6.0f, 8.0f, -6.0f }; // プレイヤーから見たカメラの相対位置
    float followSmooth_ = 10.0f; // 追従の追いつき速度
    bool enableCameraFollow_ = false; // カメラ追従の有効化
    void SetEnableCameraFollow(bool v) { enableCameraFollow_ = v; }

    // --- アタッチ時のスムージング用 ---
    bool attachSmoothingActive_ = false; // 補間中かどうかのフラグ
    float attachSmoothingElapsed_ = 0.0f; // 経過時間
    float attachDuration_ = 0.3f; // 補間にかける時間
    Vector3 attachStart_ {}; // 補間開始地点
    Vector3 attachTarget_ {}; // 補間目標地点

    // --- 手(腕)のポーズ状態 ---
    bool handHeld_ = false; // 手が物を持っている状態
    // 手の位置オフセット（プレイヤー基準）
    // X: 横方向（右正）、 Y: 高さ、 Z: 前方向
    // 値を調整してプレイヤーの横に来るように設定
    Vector3 handOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 handRotation_ = { 0.0f, 0.0f, 0.0f }; // 手の目標回転
    bool handSideRight_ = true; // 手が右側かどうか
};