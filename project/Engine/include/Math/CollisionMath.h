#pragma once
#include "Geometry.h"

namespace CollisionMath {

// 球と球の当たり判定
bool IsCollision(const Sphere &s1, const Sphere &s2);

// AABBとAABBの当たり判定
bool IsCollision(const AABB &a1, const AABB &a2);

// 球とAABBの当たり判定
bool IsCollision(const Sphere &s, const AABB &aabb);
bool IsCollision(const AABB &aabb, const Sphere &s);

// 線分と球の当たり判定
bool IsCollision(const Segment &segment, const Sphere &sphere);

} // namespace CollisionMath
