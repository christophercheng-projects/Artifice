#pragma once

#include <iostream>

#include "vec3.h"
#include "vec4.h"
#include "quat.h"

struct mat4
{
    union
    {
        float elements[4*4];
        vec4 columns[4];
    };

    mat4();
    mat4(float diagonal);

    float& get(unsigned int row, unsigned int col) { return elements[row + col * 4]; }

    static mat4 identity();

    mat4& multiply(const mat4& other);
    friend mat4 operator*(mat4 left, const mat4& right);
    mat4& operator*=(const mat4& other);

    vec3 multiply(const vec3& other) const;
    friend vec3 operator*(const mat4& left, const vec3& right);

    vec4 multiply(const vec4& other) const;
    friend vec4 operator*(const mat4& left, const vec4& right);

    mat4 inverse() const;
    mat4 transpose() const;

    static void decompose(const mat4& transform, vec3& translation, vec3& scale, mat4& rotation);

    static mat4 orthographic(float left, float right, float bottom, float top, float near, float far);
    static mat4 perspective(float aspectRatio, float fov, float near, float far);

    static mat4 translation(const vec3& translation);
    static mat4 rotation(float angle, const vec3& axis);
    static mat4 rotation(const quat& rot);
    static mat4 scale(const vec3& scale);


    friend std::ostream& operator<<(std::ostream& stream, const mat4& matrix);
};
