#pragma once
#include "Matrix4x4.h"
#include "Transform.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include <cmath>

//=========================
// Vector3
//=========================

inline float LengthSq(const Vector3 &v) {
  return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline float Length(const Vector3 &v) { return std::sqrt(LengthSq(v)); }

inline float Dot(const Vector3 &a, const Vector3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vector3 Cross(const Vector3 &a, const Vector3 &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// ゼロ除算に安全な正規化（長さが極小なら {0,0,0} を返す）
inline Vector3 SafeNormalize(const Vector3 &v, float eps = 1e-6f) {
  float lsq = LengthSq(v);
  if (lsq <= eps * eps)
    return {0.0f, 0.0f, 0.0f};
  float invLen = 1.0f / std::sqrt(lsq);
  return {v.x * invLen, v.y * invLen, v.z * invLen};
}

// そのまま正規化（ゼロに厳密でない場合）
inline Vector3 Normalize(const Vector3 &v) {
  float len = Length(v);
  return (len > 0.0f) ? Vector3{v.x / len, v.y / len, v.z / len}
                      : Vector3{0.0f, 0.0f, 0.0f};
}

inline Vector3 Lerp(const Vector3 &a, const Vector3 &b, float t) {
  return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

//=========================
// Matrix4x4
//=========================

/// <summary>
/// 単位行列作成
/// </summary>
/// <returns>単位行列</returns>
Matrix4x4 MakeIdentity4x4();

/// <summary>
/// 行列の積を求める
/// </summary>
/// <param name="matrix1">行列</param>
/// <param name="matrix2">行列</param>
/// <returns>積</returns>
Matrix4x4 Multiply(Matrix4x4 matrix1, Matrix4x4 matrix2);

// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3 &translate);

// 拡縮行列
Matrix4x4 MakeScaleMatrix(const Vector3 &scale);

/// <summary>
/// X軸回転行列作成
/// </summary>
/// <param name="rotateX"></param>
/// <returns></returns>
Matrix4x4 MakeRotateXMatrix(const float &rotateX);

/// <summary>
/// Y軸回転行列作成
/// </summary>
/// <param name="rotateY"></param>
/// <returns></returns>
Matrix4x4 MakeRotateYMatrix(const float &rotateY);

/// <summary>
/// Z軸回転行列作成
/// </summary>
/// <param name="rotateZ"></param>
/// <returns></returns>
Matrix4x4 MakeRotateZMatrix(const float &rotateZ);

/// <summary>
/// 回転行列生成
/// </summary>
/// <param name="rotate"></param>
/// <returns></returns>
Matrix4x4 MakeRotateMatrix(const Vector3 &rotate);

/// <summary>
/// アフィン行列作成
/// </summary>
/// <param name="scale">拡縮</param>
/// <param name="rotate">回転</param>
/// <param name="translate">平行移動</param>
/// <returns>アフィン行列</returns>
Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate);

/// <summary>
/// 透視投影行列作成
/// </summary>
/// <param name="fovY">画角</param>
/// <param name="aspectRatio">アスペクト比</param>
/// <param name="nearClip">近平面</param>
/// <param name="farClip">遠平面</param>
/// <returns>透視投影行列</returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip);

/// <summary>
/// 正射影行列(平行投影行列)作成
/// </summary>
/// <param name="left">左端</param>
/// <param name="top">上端</param>
/// <param name="right">右端</param>
/// <param name="bottom">下端</param>
/// <param name="nearClip">近平面</param>
/// <param name="farClip">遠平面</param>
/// <returns>正射影行列</returns>
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip);

/// <summary>
/// 4x4逆行列生成
/// </summary>
/// <param name="matrix">4x4正則行列</param>
/// <returns>逆行列</returns>
Matrix4x4 Inverse(Matrix4x4 matrix);

// 法線の変換（回転・スケールのみ適用／平行移動は無視）
Vector3 TransformNormal(const Vector3 &v, const Matrix4x4 &m);
