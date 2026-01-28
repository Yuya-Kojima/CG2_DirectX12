#pragma once
#include "Vector3.h"

struct AABB {
  Vector3 min; // 最小点
  Vector3 max; // 最大点
};

struct OBB {
  Vector3 center;      // 中心点
  Vector3 halfExtents; // 半辺長
  float yaw = 0.0f;    // Y回転（ラジアン、XZ平面回転）
};

struct AccelerationField {
  Vector3 acceleration;
  AABB area; // 範囲
};

bool IsCollision(const AABB &aabb, const Vector3 &point) {

  if (point.x < aabb.min.x || point.x > aabb.max.x)
    return false;

  if (point.y < aabb.min.y || point.y > aabb.max.y)
    return false;

  if (point.z < aabb.min.z || point.z > aabb.max.z)
    return false;

  return true;
}