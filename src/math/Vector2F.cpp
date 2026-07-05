#include "Vector2F.h"

#include <iostream>

Vector2F::Vector2F(float x, float y) : x(x), y(y) {}

void Vector2F::print() const {
    std::cout << "Vector: (" << x << ", " << y << ")" << std::endl;
}

Vector2F Vector2F::operator+(const Vector2F& other) const {
    return Vector2F(x + other.x, y + other.y);
}

Vector2F Vector2F::operator-(const Vector2F& other) const {
    return Vector2F(x - other.x, y - other.y);
}

Vector2F Vector2F::operator*(float scalar) const {
    return Vector2F(x * scalar, y * scalar);
}

Vector2F Vector2F::operator/(float scalar) const {
    return Vector2F(x / scalar, y / scalar);
}

Vector2F& Vector2F::operator+=(const Vector2F& other) {
    x += other.x;
    y += other.y;
    return *this;
}

float Vector2F::distanceSquared(const Vector2F& v1, const Vector2F& v2) {
    Vector2F diff = v1 - v2;
    return diff.x * diff.x + diff.y * diff.y;
}

Vector2F Vector2F::one() {
    return Vector2F(1.0f, 1.0f);
}

Vector2F Vector2F::zero() {
    return Vector2F(0.0f, 0.0f);
}
