#include "Math/CollisionMath.h"
#include "Math/MathUtil.h"
#include <algorithm>

namespace CollisionMath {

bool IsCollision(const Sphere &s1, const Sphere &s2) {
  // 中心点同士の距離の2乗を求める
  Vector3 diff = Subtract(s1.center, s2.center);
  float distanceSq = LengthSq(diff);

  // 半径の和の2乗と比較する
  float radiusSum = s1.radius + s2.radius;
  return distanceSq <= (radiusSum * radiusSum);
}

bool IsCollision(const AABB &a1, const AABB &a2) {
  // 全ての軸で重なっているかを確認
  if (a1.min.x <= a2.max.x && a1.max.x >= a2.min.x && a1.min.y <= a2.max.y &&
      a1.max.y >= a2.min.y && a1.min.z <= a2.max.z && a1.max.z >= a2.min.z) {
    return true;
  }
  return false;
}

bool IsCollision(const Sphere &s, const AABB &aabb) {
  // AABBの箱の中で、球の中心に最も近い点(ClosestPoint)を求める
  Vector3 closestPoint = {std::clamp(s.center.x, aabb.min.x, aabb.max.x),
                          std::clamp(s.center.y, aabb.min.y, aabb.max.y),
                          std::clamp(s.center.z, aabb.min.z, aabb.max.z)};

  // 最も近い点と球の中心の距離が球の半径以下なら当たっている
  Vector3 diff = Subtract(closestPoint, s.center);
  float distanceSq = LengthSq(diff);
  return distanceSq <= (s.radius * s.radius);
}

bool IsCollision(const AABB &aabb, const Sphere &s) {
  return IsCollision(s, aabb);
}

bool IsCollision(const Segment &segment, const Sphere &sphere) {
  // 線分の始点から球の中心へのベクトル
  Vector3 v = Subtract(sphere.center, segment.origin);

  // 線分のベクトル
  Vector3 diff = segment.diff;
  float diffSq = LengthSq(diff);

  // 線分の長さが0の場合の処理
  if (diffSq == 0.0f) {
    float distSq = LengthSq(v);
    return distSq <= (sphere.radius * sphere.radius);
  }

  // 内積を使って、球の中心を線分上に射影した時の位置(t)を求める
  // tは0.0f～ 1.0fにクランプする
  float t = Dot(v, diff) / diffSq;
  t = std::clamp(t, 0.0f, 1.0f);

  // 線分上の最も球に近い点を割り出す
  Vector3 closestPoint = Add(segment.origin, Multiply(t, diff));

  // その点と球の中心との距離が、球の半径以下なら当たっている
  Vector3 distanceVec = Subtract(closestPoint, sphere.center);
  float distanceSq = LengthSq(distanceVec);

  return distanceSq <= (sphere.radius * sphere.radius);
}

} // namespace CollisionMath
