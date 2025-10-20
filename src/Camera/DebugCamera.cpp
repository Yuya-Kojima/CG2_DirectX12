#include "Camera/DebugCamera.h"

void DebugCamera::Initialize() {
  matRot_ = MakeIdentity4x4();
  translation_ = {0.0f, 0.0f, -10.0f};
  viewMatrix_ = MakeIdentity4x4();
}

void DebugCamera::Update(const InputKeyState &input) {

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
  const float rotateSpeed = 0.05f;

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

  // 累積の回転行列を合成
  matRot_ = Multiply(matRotDelta, matRot_);
  Vector3 cameraPos = TransformNormal(translation_, matRot_);

  // 行列の更新
  Matrix4x4 translateMatrix = MakeTranslateMatrix(cameraPos);

  Matrix4x4 worldMatrix = Multiply(matRot_, translateMatrix);

  viewMatrix_ = Inverse(worldMatrix);
}
