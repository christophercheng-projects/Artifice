#pragma once

#include <iostream>


struct mat4;
struct vec3;

struct vec4
{
    float x, y, z, w;
    
    vec4();
    vec4(float v);
    vec4(float x, float y, float z, float w);
    vec4(const vec3& xyz, float w);
    
    vec4& add(const vec4& other);
    vec4& subtract(const vec4& other);
    vec4& multiply(const vec4& other);
    vec4& divide(const vec4& other);
    
    vec4 multiply(const mat4& transform) const;
    
    friend vec4 operator+(vec4 left, const vec4& right);
    friend vec4 operator-(vec4 left, const vec4& right);
    friend vec4 operator*(vec4 left, const vec4& right);
    friend vec4 operator/(vec4 left, const vec4& right);
    
    bool operator==(const vec4& other);
    bool operator!=(const vec4& other);
    
    vec4& operator+=(const vec4& other);
    vec4& operator-=(const vec4& other);
    vec4& operator*=(const vec4& other);
    vec4& operator/=(const vec4& other);
    
    float Magnitude() const;
    
    vec4 Normalize() const;
    
    friend std::ostream& operator<<(std::ostream& stream, const vec4& vector);
};
