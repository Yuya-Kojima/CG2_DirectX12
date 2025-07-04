#pragma once

/// <summary>
/// 3次元ベクトル
/// </summary>
struct Vector3 {
  float x;
  float y;
  float z;

  Vector3 &operator+=(float s) {
    x += s;
    y += s;
    z += s;
    return *this;
  }

  Vector3 &operator-=(float s) {
    x -= s;
    y -= s;
    z -= s;
    return *this;
  }

  Vector3 &operator+=(const Vector3 &v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }

  Vector3 &operator-=(const Vector3 &v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
  }
};

inline Vector3 Add(const Vector3 &v1, const Vector3 &v2) {

  Vector3 result;

  result = {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};

  return result;
}

inline Vector3 Subtract(const Vector3 &v1, const Vector3 &v2) {

  Vector3 result;

  result.x = v1.x - v2.x;
  result.y = v1.y - v2.y;
  result.z = v1.z - v2.z;

  return result;
}

inline Vector3 Multiply(const float &f, const Vector3 vector) {

  Vector3 result;

  result = {vector.x * f, vector.y * f, vector.z * f};

  return result;
}

// Vector3 operator-(const Vector3 &v) { return {-v.x, -v.y, -v.z}; }

// Vector3 operator+(const Vector3 &v) { return v; }

// Vector3 operator+(const Vector3 &v1, const Vector3 &v2) { return Add(v1, v2);
// }

// Vector3 operator-(const Vector3 &v1, const Vector3 &v2) {
// return Subtract(v1, v2);
//}

inline Vector3 operator*(float s, const Vector3 &v) { return Multiply(s, v); }

inline Vector3 operator*(const Vector3 &v, float s) { return s * v; }

// Vector3 operator/(const Vector3 &v, float s) { return Multiply(1.0f / s, v);
// }