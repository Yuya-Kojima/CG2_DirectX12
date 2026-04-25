#include "Math/MathUtil.h"

Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t) {
    float dot = q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;
    Quaternion tq1 = q1;
    if (dot < 0.0f) {
        dot = -dot;
        tq1 = { -q1.x, -q1.y, -q1.z, -q1.w };
    }
    if (dot >= 1.0f - 0.0005f) {
        return {
            q0.x + (tq1.x - q0.x) * t,
            q0.y + (tq1.y - q0.y) * t,
            q0.z + (tq1.z - q0.z) * t,
            q0.w + (tq1.w - q0.w) * t
        };
    }
    float theta = std::acos(dot);
    float sinTheta = std::sin(theta);
    float s0 = std::sin((1.0f - t) * theta) / sinTheta;
    float s1 = std::sin(t * theta) / sinTheta;
    return {
        q0.x * s0 + tq1.x * s1,
        q0.y * s0 + tq1.y * s1,
        q0.z * s0 + tq1.z * s1,
        q0.w * s0 + tq1.w * s1
    };
}

Matrix4x4 MakeIdentity4x4() {
  Matrix4x4 matrix;

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i == j) {
        matrix.m[i][j] = 1.0f;
      } else {
        matrix.m[i][j] = 0.0f;
      }
    }
  }

  return matrix;
}

Matrix4x4 Multiply(Matrix4x4 matrix1, Matrix4x4 matrix2) {

  Matrix4x4 result;

  result.m[0][0] =
      matrix1.m[0][0] * matrix2.m[0][0] + matrix1.m[0][1] * matrix2.m[1][0] +
      matrix1.m[0][2] * matrix2.m[2][0] + matrix1.m[0][3] * matrix2.m[3][0];

  result.m[0][1] =
      matrix1.m[0][0] * matrix2.m[0][1] + matrix1.m[0][1] * matrix2.m[1][1] +
      matrix1.m[0][2] * matrix2.m[2][1] + matrix1.m[0][3] * matrix2.m[3][1];

  result.m[0][2] =
      matrix1.m[0][0] * matrix2.m[0][2] + matrix1.m[0][1] * matrix2.m[1][2] +
      matrix1.m[0][2] * matrix2.m[2][2] + matrix1.m[0][3] * matrix2.m[3][2];

  result.m[0][3] =
      matrix1.m[0][0] * matrix2.m[0][3] + matrix1.m[0][1] * matrix2.m[1][3] +
      matrix1.m[0][2] * matrix2.m[2][3] + matrix1.m[0][3] * matrix2.m[3][3];

  result.m[1][0] =
      matrix1.m[1][0] * matrix2.m[0][0] + matrix1.m[1][1] * matrix2.m[1][0] +
      matrix1.m[1][2] * matrix2.m[2][0] + matrix1.m[1][3] * matrix2.m[3][0];

  result.m[1][1] =
      matrix1.m[1][0] * matrix2.m[0][1] + matrix1.m[1][1] * matrix2.m[1][1] +
      matrix1.m[1][2] * matrix2.m[2][1] + matrix1.m[1][3] * matrix2.m[3][1];

  result.m[1][2] =
      matrix1.m[1][0] * matrix2.m[0][2] + matrix1.m[1][1] * matrix2.m[1][2] +
      matrix1.m[1][2] * matrix2.m[2][2] + matrix1.m[1][3] * matrix2.m[3][2];

  result.m[1][3] =
      matrix1.m[1][0] * matrix2.m[0][3] + matrix1.m[1][1] * matrix2.m[1][3] +
      matrix1.m[1][2] * matrix2.m[2][3] + matrix1.m[1][3] * matrix2.m[3][3];

  result.m[2][0] =
      matrix1.m[2][0] * matrix2.m[0][0] + matrix1.m[2][1] * matrix2.m[1][0] +
      matrix1.m[2][2] * matrix2.m[2][0] + matrix1.m[2][3] * matrix2.m[3][0];

  result.m[2][1] =
      matrix1.m[2][0] * matrix2.m[0][1] + matrix1.m[2][1] * matrix2.m[1][1] +
      matrix1.m[2][2] * matrix2.m[2][1] + matrix1.m[2][3] * matrix2.m[3][1];

  result.m[2][2] =
      matrix1.m[2][0] * matrix2.m[0][2] + matrix1.m[2][1] * matrix2.m[1][2] +
      matrix1.m[2][2] * matrix2.m[2][2] + matrix1.m[2][3] * matrix2.m[3][2];

  result.m[2][3] =
      matrix1.m[2][0] * matrix2.m[0][3] + matrix1.m[2][1] * matrix2.m[1][3] +
      matrix1.m[2][2] * matrix2.m[2][3] + matrix1.m[2][3] * matrix2.m[3][3];

  result.m[3][0] =
      matrix1.m[3][0] * matrix2.m[0][0] + matrix1.m[3][1] * matrix2.m[1][0] +
      matrix1.m[3][2] * matrix2.m[2][0] + matrix1.m[3][3] * matrix2.m[3][0];

  result.m[3][1] =
      matrix1.m[3][0] * matrix2.m[0][1] + matrix1.m[3][1] * matrix2.m[1][1] +
      matrix1.m[3][2] * matrix2.m[2][1] + matrix1.m[3][3] * matrix2.m[3][1];

  result.m[3][2] =
      matrix1.m[3][0] * matrix2.m[0][2] + matrix1.m[3][1] * matrix2.m[1][2] +
      matrix1.m[3][2] * matrix2.m[2][2] + matrix1.m[3][3] * matrix2.m[3][2];

  result.m[3][3] =
      matrix1.m[3][0] * matrix2.m[0][3] + matrix1.m[3][1] * matrix2.m[1][3] +
      matrix1.m[3][2] * matrix2.m[2][3] + matrix1.m[3][3] * matrix2.m[3][3];

  return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3 &translate) {

  Matrix4x4 result;

  result.m[0][0] = 1.0f;
  result.m[0][1] = 0.0f;
  result.m[0][2] = 0.0f;
  result.m[0][3] = 0.0f;
  result.m[1][0] = 0.0f;
  result.m[1][1] = 1.0f;
  result.m[1][2] = 0.0f;
  result.m[1][3] = 0.0f;
  result.m[2][0] = 0.0f;
  result.m[2][1] = 0.0f;
  result.m[2][2] = 1.0f;
  result.m[2][3] = 0.0f;
  result.m[3][0] = translate.x;
  result.m[3][1] = translate.y;
  result.m[3][2] = translate.z;
  result.m[3][3] = 1.0f;

  return result;
}

Matrix4x4 MakeScaleMatrix(const Vector3 &scale) {

  Matrix4x4 result;

  result.m[0][0] = scale.x;
  result.m[0][1] = 0.0f;
  result.m[0][2] = 0.0f;
  result.m[0][3] = 0.0f;
  result.m[1][0] = 0.0f;
  result.m[1][1] = scale.y;
  result.m[1][2] = 0.0f;
  result.m[1][3] = 0.0f;
  result.m[2][0] = 0.0f;
  result.m[2][1] = 0.0f;
  result.m[2][2] = scale.z;
  result.m[2][3] = 0.0f;
  result.m[3][0] = 0.0f;
  result.m[3][1] = 0.0f;
  result.m[3][2] = 0.0f;
  result.m[3][3] = 1.0f;

  return result;
}

Matrix4x4 MakeRotateXMatrix(const float &rotateX) {

  Matrix4x4 result;
  result.m[0][0] = 1.0f;
  result.m[0][1] = 0.0f;
  result.m[0][2] = 0.0f;
  result.m[0][3] = 0.0f;
  result.m[1][0] = 0.0f;
  result.m[1][1] = cosf(rotateX);
  result.m[1][2] = sinf(rotateX);
  result.m[1][3] = 0.0f;
  result.m[2][0] = 0.0f;
  result.m[2][1] = -sinf(rotateX);
  result.m[2][2] = cosf(rotateX);
  result.m[2][3] = 0.0f;
  result.m[3][0] = 0.0f;
  result.m[3][1] = 0.0f;
  result.m[3][2] = 0.0f;
  result.m[3][3] = 1.0f;

  return result;
}

Matrix4x4 MakeRotateYMatrix(const float &rotateY) {

  Matrix4x4 result;
  result.m[0][0] = cosf(rotateY);
  result.m[0][1] = 0.0f;
  result.m[0][2] = -sinf(rotateY);
  result.m[0][3] = 0.0f;
  result.m[1][0] = 0.0f;
  result.m[1][1] = 1.0f;
  result.m[1][2] = 0.0f;
  result.m[1][3] = 0.0f;
  result.m[2][0] = sinf(rotateY);
  result.m[2][1] = 0.0f;
  result.m[2][2] = cosf(rotateY);
  result.m[2][3] = 0.0f;
  result.m[3][0] = 0.0f;
  result.m[3][1] = 0.0f;
  result.m[3][2] = 0.0f;
  result.m[3][3] = 1.0f;

  return result;
}

Matrix4x4 MakeRotateZMatrix(const float &rotateZ) {

  Matrix4x4 result;
  result.m[0][0] = cosf(rotateZ);
  result.m[0][1] = sinf(rotateZ);
  result.m[0][2] = 0.0f;
  result.m[0][3] = 0.0f;
  result.m[1][0] = -sinf(rotateZ);
  result.m[1][1] = cosf(rotateZ);
  result.m[1][2] = 0.0f;
  result.m[1][3] = 0.0f;
  result.m[2][0] = 0.0f;
  result.m[2][1] = 0.0f;
  result.m[2][2] = 1.0f;
  result.m[2][3] = 0.0f;
  result.m[3][0] = 0.0f;
  result.m[3][1] = 0.0f;
  result.m[3][2] = 0.0f;
  result.m[3][3] = 1.0f;

  return result;
}

Matrix4x4 MakeRotateMatrix(const Vector3 &rotate) {

  Matrix4x4 rotateX = MakeRotateXMatrix(rotate.x);

  Matrix4x4 rotateY = MakeRotateYMatrix(rotate.y);

  Matrix4x4 rotateZ = MakeRotateZMatrix(rotate.z);

  Matrix4x4 result = Multiply(rotateX, Multiply(rotateY, rotateZ));

  return result;
}

Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate) {
  Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

  Matrix4x4 rotateMatrix = MakeRotateMatrix(rotate);

  Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

  Matrix4x4 worldMatrix;
  worldMatrix = Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);

  return worldMatrix;
}

Matrix4x4 MakeRotateMatrix(const Quaternion& quaternion) {
    Matrix4x4 result = MakeIdentity4x4();
    float xx = quaternion.x * quaternion.x;
    float yy = quaternion.y * quaternion.y;
    float zz = quaternion.z * quaternion.z;
    float ww = quaternion.w * quaternion.w;
    float xy = quaternion.x * quaternion.y;
    float xz = quaternion.x * quaternion.z;
    float xw = quaternion.x * quaternion.w;
    float yz = quaternion.y * quaternion.z;
    float yw = quaternion.y * quaternion.w;
    float zw = quaternion.z * quaternion.w;

    result.m[0][0] = ww + xx - yy - zz;
    result.m[0][1] = 2.0f * (xy + zw);
    result.m[0][2] = 2.0f * (xz - yw);
    result.m[0][3] = 0.0f;

    result.m[1][0] = 2.0f * (xy - zw);
    result.m[1][1] = ww - xx + yy - zz;
    result.m[1][2] = 2.0f * (yz + xw);
    result.m[1][3] = 0.0f;

    result.m[2][0] = 2.0f * (xz + yw);
    result.m[2][1] = 2.0f * (yz - xw);
    result.m[2][2] = ww - xx - yy + zz;
    result.m[2][3] = 0.0f;

    result.m[3][0] = 0.0f;
    result.m[3][1] = 0.0f;
    result.m[3][2] = 0.0f;
    result.m[3][3] = 1.0f;

    return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate) {
    Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
    Matrix4x4 rotateMatrix = MakeRotateMatrix(rotate);
    Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

    Matrix4x4 worldMatrix = Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);
    return worldMatrix;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip) {
  Matrix4x4 matrix;
  float f = 1.0f / tanf(fovY / 2.0f);

  matrix.m[0][0] = f / aspectRatio;
  matrix.m[0][1] = 0.0f;
  matrix.m[0][2] = 0.0f;
  matrix.m[0][3] = 0.0f;
  matrix.m[1][0] = 0.0f;
  matrix.m[1][1] = f;
  matrix.m[1][2] = 0.0f;
  matrix.m[1][3] = 0.0f;
  matrix.m[2][0] = 0.0f;
  matrix.m[2][1] = 0.0f;
  matrix.m[2][2] = farClip / (farClip - nearClip);
  matrix.m[2][3] = 1.0f;
  matrix.m[3][0] = 0.0f;
  matrix.m[3][1] = 0.0f;
  matrix.m[3][2] = -nearClip * farClip / (farClip - nearClip);
  matrix.m[3][3] = 0.0f;

  return matrix;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip) {
  Matrix4x4 matrix;

  matrix.m[0][0] = 2.0f / (right - left);
  matrix.m[0][1] = 0.0f;
  matrix.m[0][2] = 0.0f;
  matrix.m[0][3] = 0.0f;
  matrix.m[1][0] = 0.0f;
  matrix.m[1][1] = 2.0f / (top - bottom);
  matrix.m[1][2] = 0.0f;
  matrix.m[1][3] = 0.0f;
  matrix.m[2][0] = 0.0f;
  matrix.m[2][1] = 0.0f;
  matrix.m[2][2] = 1.0f / (farClip - nearClip);
  matrix.m[2][3] = 0.0f;
  matrix.m[3][0] = (left + right) / (left - right);
  matrix.m[3][1] = (top + bottom) / (bottom - top);
  matrix.m[3][2] = nearClip / (nearClip - farClip);
  matrix.m[3][3] = 1.0f;

  return matrix;
}

Matrix4x4 Inverse(Matrix4x4 matrix) {
  Matrix4x4 inverseMatrix;

  float det =
      matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][3] +
      matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][1] +
      matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][2] -
      matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][1] -
      matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][3] -
      matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][2] -
      matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][3] -
      matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][1] -
      matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][2] +
      matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][1] +
      matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][3] +
      matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][2] +
      matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][3] +
      matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][1] +
      matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][2] -
      matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][1] -
      matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][3] -
      matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][2] -
      matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][0] -
      matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][0] -
      matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][0] +
      matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][0] +
      matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][0] +
      matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][0];

  float invDet = 1 / det;

  // 逆行列を求めるための補助行列を計算
  inverseMatrix.m[0][0] = (matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][3] -
                           matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][2] -
                           matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][3] +
                           matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][1] +
                           matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][2] -
                           matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][1]) *
                          invDet;

  inverseMatrix.m[0][1] = (-(matrix.m[0][1] * matrix.m[2][2] * matrix.m[3][3]) +
                           matrix.m[0][1] * matrix.m[2][3] * matrix.m[3][2] +
                           matrix.m[0][2] * matrix.m[2][1] * matrix.m[3][3] -
                           matrix.m[0][2] * matrix.m[2][3] * matrix.m[3][1] -
                           matrix.m[0][3] * matrix.m[2][1] * matrix.m[3][2] +
                           matrix.m[0][3] * matrix.m[2][2] * matrix.m[3][1]) *
                          invDet;

  inverseMatrix.m[0][2] = (matrix.m[0][1] * matrix.m[1][2] * matrix.m[3][3] -
                           matrix.m[0][1] * matrix.m[1][3] * matrix.m[3][2] -
                           matrix.m[0][2] * matrix.m[1][1] * matrix.m[3][3] +
                           matrix.m[0][2] * matrix.m[1][3] * matrix.m[3][1] +
                           matrix.m[0][3] * matrix.m[1][1] * matrix.m[3][2] -
                           matrix.m[0][3] * matrix.m[1][2] * matrix.m[3][1]) *
                          invDet;

  inverseMatrix.m[0][3] = (-(matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][3]) +
                           matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][2] +
                           matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][3] -
                           matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][1] -
                           matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][2] +
                           matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][1]) *
                          invDet;
  inverseMatrix.m[1][0] = (-(matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][3]) +
                           matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][2] +
                           matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][3] -
                           matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][0] -
                           matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][2] +
                           matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[1][1] = (matrix.m[0][0] * matrix.m[2][2] * matrix.m[3][3] -
                           matrix.m[0][0] * matrix.m[2][3] * matrix.m[3][2] -
                           matrix.m[0][2] * matrix.m[2][0] * matrix.m[3][3] +
                           matrix.m[0][2] * matrix.m[2][3] * matrix.m[3][0] +
                           matrix.m[0][3] * matrix.m[2][0] * matrix.m[3][2] -
                           matrix.m[0][3] * matrix.m[2][2] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[1][2] = (-(matrix.m[0][0] * matrix.m[1][2] * matrix.m[3][3]) +
                           matrix.m[0][0] * matrix.m[1][3] * matrix.m[3][2] +
                           matrix.m[0][2] * matrix.m[1][0] * matrix.m[3][3] -
                           matrix.m[0][2] * matrix.m[1][3] * matrix.m[3][0] -
                           matrix.m[0][3] * matrix.m[1][0] * matrix.m[3][2] +
                           matrix.m[0][3] * matrix.m[1][2] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[1][3] = (matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][3] -
                           matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][2] -
                           matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][3] +
                           matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][0] +
                           matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][2] -
                           matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][0]) *
                          invDet;

  inverseMatrix.m[2][0] = (matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][3] -
                           matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][1] -
                           matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][3] +
                           matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][0] +
                           matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][1] -
                           matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[2][1] = (-(matrix.m[0][0] * matrix.m[2][1] * matrix.m[3][3]) +
                           matrix.m[0][0] * matrix.m[2][3] * matrix.m[3][1] +
                           matrix.m[0][1] * matrix.m[2][0] * matrix.m[3][3] -
                           matrix.m[0][1] * matrix.m[2][3] * matrix.m[3][0] -
                           matrix.m[0][3] * matrix.m[2][0] * matrix.m[3][1] +
                           matrix.m[0][3] * matrix.m[2][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[2][2] = (matrix.m[0][0] * matrix.m[1][1] * matrix.m[3][3] -
                           matrix.m[0][0] * matrix.m[1][3] * matrix.m[3][1] -
                           matrix.m[0][1] * matrix.m[1][0] * matrix.m[3][3] +
                           matrix.m[0][1] * matrix.m[1][3] * matrix.m[3][0] +
                           matrix.m[0][3] * matrix.m[1][0] * matrix.m[3][1] -
                           matrix.m[0][3] * matrix.m[1][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[2][3] = (-(matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][3]) +
                           matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][1] +
                           matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][3] -
                           matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][0] -
                           matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][1] +
                           matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][0]) *
                          invDet;

  inverseMatrix.m[3][0] = (-(matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][2]) +
                           matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][1] +
                           matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][2] -
                           matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][0] -
                           matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][1] +
                           matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[3][1] = (matrix.m[0][0] * matrix.m[2][1] * matrix.m[3][2] -
                           matrix.m[0][0] * matrix.m[2][2] * matrix.m[3][1] -
                           matrix.m[0][1] * matrix.m[2][0] * matrix.m[3][2] +
                           matrix.m[0][1] * matrix.m[2][2] * matrix.m[3][0] +
                           matrix.m[0][2] * matrix.m[2][0] * matrix.m[3][1] -
                           matrix.m[0][2] * matrix.m[2][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[3][2] = (-(matrix.m[0][0] * matrix.m[1][1] * matrix.m[3][2]) +
                           matrix.m[0][0] * matrix.m[1][2] * matrix.m[3][1] +
                           matrix.m[0][1] * matrix.m[1][0] * matrix.m[3][2] -
                           matrix.m[0][1] * matrix.m[1][2] * matrix.m[3][0] -
                           matrix.m[0][2] * matrix.m[1][0] * matrix.m[3][1] +
                           matrix.m[0][2] * matrix.m[1][1] * matrix.m[3][0]) *
                          invDet;

  inverseMatrix.m[3][3] = (matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] -
                           matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1] -
                           matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] +
                           matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] +
                           matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1] -
                           matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0]) *
                          invDet;

  return inverseMatrix;
}

Vector3 TransformNormal(const Vector3 &v, const Matrix4x4 &m) {
  Vector3 result;
  result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
  result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
  result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];
  return result;
}

Matrix4x4 Transpose(Matrix4x4 matrix) {
  Matrix4x4 result{};

  result.m[0][0] = matrix.m[0][0];
  result.m[0][1] = matrix.m[1][0];
  result.m[0][2] = matrix.m[2][0];
  result.m[0][3] = matrix.m[3][0];

  result.m[1][0] = matrix.m[0][1];
  result.m[1][1] = matrix.m[1][1];
  result.m[1][2] = matrix.m[2][1];
  result.m[1][3] = matrix.m[3][1];

  result.m[2][0] = matrix.m[0][2];
  result.m[2][1] = matrix.m[1][2];
  result.m[2][2] = matrix.m[2][2];
  result.m[2][3] = matrix.m[3][2];

  result.m[3][0] = matrix.m[0][3];
  result.m[3][1] = matrix.m[1][3];
  result.m[3][2] = matrix.m[2][3];
  result.m[3][3] = matrix.m[3][3];

  return result;
}
