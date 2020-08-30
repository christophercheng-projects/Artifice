#pragma once

#include <iostream>


struct vec2;
struct vec4;
struct mat4;

struct vec3
{
    float x, y, z;

    vec3();
    vec3(float v);
    vec3(float x, float y, float z);
    vec3(const vec2& xy, float z);
    vec3(const vec4& xyzw);

    vec3& add(const vec3& other);
    vec3& subtract(const vec3& other);
    vec3& multiply(const vec3& other);
    vec3& divide(const vec3& other);

    vec3& multiply(float other);

    vec3 multiply(const mat4& transform) const;

    friend vec3 operator+(vec3 left, const vec3& right);
    friend vec3 operator-(vec3 left, const vec3& right);
    friend vec3 operator*(vec3 left, const vec3& right);
    friend vec3 operator/(vec3 left, const vec3& right);

    friend vec3 operator*(vec3 left, float right);
    friend vec3 operator*(float left, vec3 right);

    bool operator==(const vec3& other);
    bool operator!=(const vec3& other);

    vec3& operator+=(const vec3& other);
    vec3& operator-=(const vec3& other);
    vec3& operator*=(const vec3& other);
    vec3& operator/=(const vec3& other);

    vec3 Cross(const vec3& other) const;
    float Dot(const vec3& other) const;
    float Magnitude() const;
    vec3 Normalize() const;

    float Distance(const vec3& other) const;

    friend std::ostream& operator<<(std::ostream& stream, const vec3& vector);
};
