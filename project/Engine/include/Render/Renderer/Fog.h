#pragma once

#include "Math/MathUtil.h"

struct FogData {
    Vector4 color;    // 霧の色（RGBA）
    float nearDist;   // 霧がかかり始める距離
    float farDist;    // 完全に霧の色になる距離
    float enabled;    // 1.0f = 有効, 0.0f = 無効
    float padding;    // 16バイトアライメント用
};
