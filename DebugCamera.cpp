#include "DebugCamera.h"

void DebugCamera::Initialize() {
  matRot_ = MakeIdentity4x4();
  translation_ = {0.0f, 0.0f, -50.0f};
  viewMatrix_ = MakeIdentity4x4();
}

void DebugCamera::Update(const InputKeyState &input) {

  Gamepad::Update();

  if (input.IsPressKey(DIK_E)) {

    //==================
    // 前入力
    //==================

    const float speed = 0.3f;

    // カメラ位置ベクトル
    Vector3 move = {0, 0, speed};

    // 移動ベクトルを角度分だけ回転させる
    move = TransformNormal(move, matRot_);

    // 移動ベクトル分だけ座標を加算する
    translation_ = Add(translation_, move);
  } else if (input.IsPressKey(DIK_Q)) {

    //==================
    // 後ろ入力
    //==================

    const float speed = -0.3f;

    // カメラ位置ベクトル
    Vector3 move = {0, 0, speed};

    // 移動ベクトルを角度分だけ回転させる
    move = TransformNormal(move, matRot_);

    // 移動ベクトル分だけ座標を加算する
    translation_ = Add(translation_, move);
  } else if (input.IsPressKey(DIK_A)) {

    //==================
    // 左入力
    //==================

    const float speed = -0.3f;

    // カメラ位置ベクトル
    Vector3 move = {speed, 0, 0};

    // 移動ベクトルを角度分だけ回転させる
    move = TransformNormal(move, matRot_);

    // 移動ベクトル分だけ座標を加算する
    translation_ = Add(translation_, move);
  } else if (input.IsPressKey(DIK_D)) {

    //==================
    // 右入力
    //==================

    const float speed = 0.3f;

    // カメラ位置ベクトル
    Vector3 move = {speed, 0, 0};

    // 移動ベクトルを角度分だけ回転させる
    move = TransformNormal(move, matRot_);

    // 移動ベクトル分だけ座標を加算する
    translation_ = Add(translation_, move);
  } else if (input.IsPressKey(DIK_W)) {

    //==================
    // 上入力
    //==================

    const float speed = 0.3f;

    // カメラ位置ベクトル
    Vector3 move = {0, speed, 0};

    // 移動ベクトルを角度分だけ回転させる
    move = TransformNormal(move, matRot_);

    // 移動ベクトル分だけ座標を加算する
    translation_ = Add(translation_, move);
  } else if (input.IsPressKey(DIK_S)) {

    //==================
    // 下入力
    //==================

    const float speed = -0.3f;

    // カメラ位置ベクトル
    Vector3 move = {0, speed, 0};

    // 移動ベクトルを角度分だけ回転させる
    move = TransformNormal(move, matRot_);

    // 移動ベクトル分だけ座標を加算する
    translation_ = Add(translation_, move);
  }

  // 追加回転分の回転行列を生成
  Matrix4x4 matRotDelta = MakeIdentity4x4();
  const float rotateSpeed = 0.01f;

  if (input.IsPressKey(DIK_C)) {
    matRotDelta = Multiply(MakeRotateYMatrix(rotateSpeed), matRotDelta);
  }

  if (input.IsPressKey(DIK_Z)) {
    matRotDelta = Multiply(MakeRotateYMatrix(-rotateSpeed), matRotDelta);
  }

  if (input.IsPressKey(DIK_UP)) {
    matRotDelta = Multiply(MakeRotateXMatrix(-rotateSpeed), matRotDelta);
  }

  if (input.IsPressKey(DIK_DOWN)) {
    matRotDelta = Multiply(MakeRotateXMatrix(rotateSpeed), matRotDelta);
  }

  if (input.IsPressKey(DIK_RIGHT)) {
    matRotDelta = Multiply(MakeRotateZMatrix(rotateSpeed), matRotDelta);
  }

  if (input.IsPressKey(DIK_LEFT)) {
    matRotDelta = Multiply(MakeRotateZMatrix(-rotateSpeed), matRotDelta);
  }

  // -------------------------
  // ゲームパッドによる移動処理
  // -------------------------

  const float moveSpeed = 0.3f;
  Gamepad::Stick left = Gamepad::GetNormalizedLeftStick();

  // 左スティックで上下左右に移動
  Vector3 move = {
      left.x * moveSpeed,
      left.y * moveSpeed,
      0.0f,
  };

  // 移動方向を回転
  move = TransformNormal(move, matRot_);
  translation_ = Add(translation_, move);

  // RT:前進
  float rt = Gamepad::GetRightTriggerStrength();
  if (rt > 0.1f) {
    const float speed = 0.3f * rt;
    Vector3 move = {0, 0, speed};
    move = TransformNormal(move, matRot_);
    translation_ = Add(translation_, move);
  }

  // LT:後退
  float lt = Gamepad::GetLeftTriggerStrength();
  if (lt > 0.1f) {
    const float speed = -0.3f * lt;
    Vector3 move = {0, 0, speed};
    move = TransformNormal(move, matRot_);
    translation_ = Add(translation_, move);
  }

  // -------------------------
  // ゲームパッドによる回転処理
  // -------------------------

  Gamepad::Stick right = Gamepad::GetNormalizedRightStick();

  // Pitch (X軸) : 上下
  if (abs(right.y) > 0.1f) {
    matRotDelta =
        Multiply(MakeRotateXMatrix(-right.y * rotateSpeed), matRotDelta);
  }

  // Yaw (Y軸) : 左右
  if (abs(right.x) > 0.1f) {
    matRotDelta =
        Multiply(MakeRotateYMatrix(right.x * rotateSpeed), matRotDelta);
  }

  // Z軸回転：LB / RB
  if (Gamepad::IsButtonPressed(XINPUT_GAMEPAD_LEFT_SHOULDER)) {
    matRotDelta = Multiply(MakeRotateZMatrix(-rotateSpeed), matRotDelta);
  }

  if (Gamepad::IsButtonPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
    matRotDelta = Multiply(MakeRotateZMatrix(rotateSpeed), matRotDelta);
  }

  // 累積の回転行列を合成
  matRot_ = Multiply(matRotDelta, matRot_);
  Vector3 cameraPos = translation_;

  // 行列の更新
  Matrix4x4 translateMatrix = MakeTranslateMatrix(cameraPos);

  Matrix4x4 worldMatrix = Multiply(matRot_, translateMatrix);

  viewMatrix_ = Inverse(worldMatrix);
}
