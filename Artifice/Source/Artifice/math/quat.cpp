#include "quat.h"

#include "math_functions.h"

quat::quat()
    : x(0.0f), y(0.0f), z(0.0f), w(1.0f)
{
}

quat::quat(float x, float y, float z, float w)
    : x(x), y(y), z(z), w(w)
{
}

quat quat::identity()
{
    return quat(0.0f, 0.0f, 0.0f, 1.0f);
}

quat& quat::add(const quat& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;

    return *this;
}

quat& quat::subtract(const quat& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;

    return *this;
}

quat& quat::multiply(const quat& other)
{
    // TODO

    return *this;
}

quat& quat::divide(const quat& other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;

    return *this;
}

quat operator+(quat left, const quat& right)
{
    return left.add(right);
}

quat operator-(quat left, const quat& right)
{
    return left.subtract(right);
}

quat operator*(quat left, const quat& right)
{
    return left.multiply(right);
}

quat operator/(quat left, const quat& right)
{
    return left.divide(right);
}

quat& quat::operator+=(const quat& other)
{
    return add(other);
}

quat& quat::operator-=(const quat& other)
{
    return subtract(other);
}

quat& quat::operator*=(const quat& other)
{
    return multiply(other);
}

quat& quat::operator/=(const quat& other)
{
    return divide(other);
}

bool quat::operator==(const quat& other)
{
    return (x == other.x && y == other.y && z == other.z && w == other.w);
}

bool quat::operator!=(const quat& other)
{
    return !(*this == other);
}

float quat::norm_squared() const 
{
    return x * x + y * y + z * z + w * w;
}

float quat::norm() const
{
    return math::sqrt(norm_squared());
}

std::ostream& operator<<(std::ostream& stream, const quat& vector)
{
    stream << "quat: (" << vector.x << ", " << vector.y << ", " << vector.z << ", " << vector.w << ")";
    return stream;
}
