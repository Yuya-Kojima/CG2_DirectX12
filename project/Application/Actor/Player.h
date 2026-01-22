#pragma once
#include "Math/Vector3.h"
#include "Camera/GameCamera.h"
#include <cstdint>

class Object3d;
class Object3dRenderer;
#include "Actor/Level.h"
#include "Input/InputKeyState.h"

class Player {
public:
    Player();
    ~Player();

    // エンジンの入力管理と Object3dRenderer を受け取って初期化する
    void Initialize(InputKeyState* input, Object3dRenderer* object3dRenderer);

    // 更新と描画
    void Update(float deltaTime);
    void Draw();

    // 位置の取得と設定
    const Vector3& GetPosition() const { return position_; }
    // カメラをアタッチする
    void AttachCamera(GameCamera* camera) { camera_ = camera; }
    // 位置の設定
    void SetPosition(const Vector3& pos);
    // レベル参照を設定（衝突判定に使用）
    void AttachLevel(class Level* level);

private:
    GameCamera* camera_ = nullptr;      // 追従用カメラ
    InputKeyState* input_ = nullptr;    // 入力管理
    // 描画用 3D オブジェクト（存在すれば Player が所有して解放する）
    Object3d* obj_ = nullptr;

    Vector3 position_ { 0.0f, 0.0f, 0.0f }; // 位置
    Vector3 velocity_ { 0.0f, 0.0f, 0.0f }; // 速度
    float rotate_ = 0.0f; // Y 軸回りの回転角度（ラジアン）

    float moveSpeed_ = 6.0f; // 移動速度
    float accel_ = 20.0f; // 加速度
    float decel_ = 25.0f; // 減速度
    float gravity_ = 30.0f; // 重力加速度
    float jumpSpeed_ = 12.0f; // ジャンプ初速
    bool onGround_ = true; // 接地フラグ
    float halfSize_ = 0.5f; // 衝突判定用の半径
    // レベル参照（衝突判定用）
    Level* level_ = nullptr;
};
