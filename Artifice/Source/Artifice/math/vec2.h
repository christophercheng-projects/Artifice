#pragma once

#include <iostream>


struct vec2
{
    float x, y;

    vec2();
    vec2(float v);
    vec2(float x, float y);

    vec2& add(const vec2& other);
    vec2& subtract(const vec2& other);
    vec2& multiply(const vec2& other);
    vec2& divide(const vec2& other);

    friend vec2 operator+(vec2 left, const vec2& right);
    friend vec2 operator-(vec2 left, const vec2& right);
    friend vec2 operator*(vec2 left, const vec2& right);
    friend vec2 operator/(vec2 left, const vec2& right);

    vec2& multiply(float other);

    friend vec2 operator*(vec2 left, float right);
    friend vec2 operator*(float left, vec2 right);

    vec2& operator*=(float other);

    bool operator==(const vec2& other);
    bool operator!=(const vec2& other);

    vec2& operator+=(const vec2& other);
    vec2& operator-=(const vec2& other);
    vec2& operator*=(const vec2& other);
    vec2& operator/=(const vec2& other);

    friend std::ostream& operator<<(std::ostream& stream, const vec2& vector);
};
