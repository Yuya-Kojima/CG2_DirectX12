#pragma once
#include "Vector3.h"

// 球 (Sphere)
struct Sphere {
  Vector3 center; // 中心点
  float radius;   // 半径
};

// 軸平行境界箱 (Axis Aligned Bounding Box)
struct AABB {
  Vector3 min; // 最小点
  Vector3 max; // 最大点
};

// 有向境界箱 (Oriented Bounding Box)
struct OBB {
  Vector3 center;          // 中心点
  Vector3 orientations[3]; // X, Y, Zのローカル軸（回転）
  Vector3 size;            // 各軸の半分の長さ(Half extents)
};

// 線分 (Segment)
struct Segment {
  Vector3 origin; // 始点
  Vector3 diff;   // 終点へのベクトル
};

// 半直線 (Ray)
struct Ray {
  Vector3 origin; // 始点
  Vector3 diff;   // 方向ベクトル
};
