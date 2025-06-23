#include "DebugCamera.h"

void DebugCamera::Update(const InputKeyState &input) {

  if (input.IsPressKey(DIK_E)) {

    //==================
    // 前入力
    //==================

    const float speed = 0.3f;

    // カメラ位置ベクトル
    Vector3 move = {0, 0, speed};

    // 移動ベクトルを角度分だけ回転させる
    Matrix4x4 rotate = MakeRotateMatrix(rotation_);

    move = TransformNormal(move, rotate);

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
    Matrix4x4 rotate = MakeRotateMatrix(rotation_);

    move = TransformNormal(move, rotate);

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
    Matrix4x4 rotate = MakeRotateMatrix(rotation_);

    move = TransformNormal(move, rotate);

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
    Matrix4x4 rotate = MakeRotateMatrix(rotation_);

    move = TransformNormal(move, rotate);

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
    Matrix4x4 rotate = MakeRotateMatrix(rotation_);

    move = TransformNormal(move, rotate);

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
    Matrix4x4 rotate = MakeRotateMatrix(rotation_);

    move = TransformNormal(move, rotate);

    // 移動ベクトル分だけ座標を加算する
    translation_ = Add(translation_, move);
  }

  const float rotateSpeed = 0.05f;

  if (input.IsPressKey(DIK_C)) {
    rotation_.x += rotateSpeed;
  }

  if (input.IsPressKey(DIK_X)) {
    rotation_.y += rotateSpeed;
  }

  if (input.IsPressKey(DIK_Z)) {
    rotation_.z += rotateSpeed;
  }

  // 行列の更新
  Matrix4x4 worldMatrix =
      MakeAffineMatrix({1.0f, 1.0f, 1.0f}, rotation_, translation_);
  viewMatrix_ = Inverse(worldMatrix);
}
