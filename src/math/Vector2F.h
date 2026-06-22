#pragma once

class Vector2F {
public:
    float x;
    float y;

    Vector2F(float x, float y);

    void print() const;

    Vector2F operator+(const Vector2F& other) const;
    Vector2F operator-(const Vector2F& other) const;
    Vector2F operator*(float scalar) const;
    Vector2F operator/(float scalar) const;

    static float distanceSquared(const Vector2F& v1, const Vector2F& v2);
    static Vector2F one();
    static Vector2F zero();
};
