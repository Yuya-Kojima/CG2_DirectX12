#pragma once

struct Vector2 {
  float x;
  float y;

  Vector2 &operator+=(float s) {
    x += s;
    y += s;
    return *this;
  }

  Vector2 &operator+=(const Vector2 &v) {
    x += v.x;
    y += v.y;
    return *this;
  }
};

Vector2 operator*(const Vector2 &v, float s) { return {v.x * s, v.y * s}; }

Vector2 operator*(float s, const Vector2 &v) { return v * s; }