#pragma once

#include <iostream>

struct mat4;

struct quat
{
    float x, y, z, w;

    quat();
    quat(float x, float y, float z, float w);

    static quat identity();

    quat& add(const quat& other);
    quat& subtract(const quat& other);
    quat& multiply(const quat& other);
    quat& divide(const quat& other);

    friend quat operator+(quat left, const quat& right);
    friend quat operator-(quat left, const quat& right);
    friend quat operator*(quat left, const quat& right);
    friend quat operator/(quat left, const quat& right);

    bool operator==(const quat& other);
    bool operator!=(const quat& other);

    quat& operator+=(const quat& other);
    quat& operator-=(const quat& other);
    quat& operator*=(const quat& other);
    quat& operator/=(const quat& other);

    float norm_squared() const;
    float norm() const;

    friend std::ostream&
    operator<<(std::ostream& stream, const quat& vector);
};
