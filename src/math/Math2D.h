#pragma once

#include "math/Vector2F.h"

#include <cmath>

namespace Math2D {
inline constexpr float DegreesToRadians = 3.14159265358979323846f / 180.0f;

inline Vector2F rotate(const Vector2F& value, float rotationDegrees) {
    const float radians = rotationDegrees * DegreesToRadians;
    const float cosAngle = std::cos(radians);
    const float sinAngle = std::sin(radians);
    return Vector2F(
        value.x * cosAngle - value.y * sinAngle,
        value.x * sinAngle + value.y * cosAngle
    );
}

inline Vector2F inverseRotate(const Vector2F& value, float rotationDegrees) {
    return rotate(value, -rotationDegrees);
}
}
