#pragma once
#include "Vector3.h"
#include "Quaternion.h"

struct EulerTransform {
  Vector3 scale;
  Vector3 rotate;
  Vector3 translate;
};

using Transform = EulerTransform;

struct QuaternionTransform {
  Vector3 scale;
  Quaternion rotate;
  Vector3 translate;
};