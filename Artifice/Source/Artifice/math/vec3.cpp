#include "vec3.h"

#include "vec2.h"
#include "vec4.h"
#include "mat4.h"
#include "math_functions.h"


vec3::vec3()
: x(0.0f), y(0.0f), z(0.0f)
{

}

vec3::vec3(float v)
: x(v), y(v), z(v)
{

}

vec3::vec3(float x, float y, float z)
: x(x), y(y), z(z)
{

}

vec3::vec3(const vec2& xy, float z)
: x(xy.x), y(xy.y), z(z)
{

}
vec3::vec3(const vec4& xyzw)
: x(xyzw.x), y(xyzw.y), z(xyzw.z)
{

}

vec3& vec3::add(const vec3& other)
{
    x += other.x;
    y += other.y;
    z += other.z;

    return *this;
}

vec3& vec3::subtract(const vec3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;

    return *this;
}

vec3& vec3::multiply(const vec3& other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;

    return *this;
}

vec3& vec3::divide(const vec3& other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;

    return *this;
}

vec3& vec3::multiply(float other)
{
    x *= other;
    y *= other;
    z *= other;

    return *this;
}

vec3 vec3::multiply(const mat4& transform) const
{
    return vec3(
                transform.elements[0 + 0 * 4] * x +
                transform.elements[0 + 1 * 4] * y +
                transform.elements[0 + 2 * 4] * z +
                transform.elements[0 + 3 * 4],

                transform.elements[1 + 0 * 4] * x +
                transform.elements[1 + 1 * 4] * y +
                transform.elements[1 + 2 * 4] * z +
                transform.elements[1 + 3 * 4],

                transform.elements[2 + 0 * 4] * x +
                transform.elements[2 + 1 * 4] * y +
                transform.elements[2 + 2 * 4] * z +
                transform.elements[2 + 3 * 4]
                );
}

vec3 operator+(vec3 left, const vec3& right)
{
    return left.add(right);
}

vec3 operator-(vec3 left, const vec3& right)
{
    return left.subtract(right);
}

vec3 operator*(vec3 left, const vec3& right)
{
    return left.multiply(right);
}

vec3 operator/(vec3 left, const vec3& right)
{
    return left.divide(right);
}

vec3 operator*(vec3 left, float right)
{
    return left.multiply(right);
}

vec3 operator*(float left, vec3 right)
{
    return right.multiply(left);
}

vec3& vec3::operator+=(const vec3& other)
{
    return add(other);
}

vec3& vec3::operator-=(const vec3& other)
{
    return subtract(other);
}

vec3& vec3::operator*=(const vec3& other)
{
    return multiply(other);
}

vec3& vec3::operator/=(const vec3& other)
{
    return divide(other);
}

bool vec3::operator==(const vec3& other)
{
    return (x == other.x && y == other.y && z == other.z);
}

bool vec3::operator!=(const vec3& other)
{
    return !(*this == other);
}

vec3 vec3::Cross(const vec3& other) const
{
    return vec3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
}

float vec3::Dot(const vec3& other) const
{
    return x * other.x + y * other.y + z * other.z;
}


float vec3::Magnitude() const
{
    return sqrt(x * x + y * y + z * z);
}

vec3 vec3::Normalize() const
{
    float length = Magnitude();
    return vec3(x / length, y / length, z / length);
}

float vec3::Distance(const vec3& other) const
{
    float a = x - other.x;
    float b = y - other.y;
    float c = z - other.z;
    return sqrt(a * a + b * b + c * c);
}

std::ostream& operator<<(std::ostream& stream, const vec3& vector)
{
    stream << "vec3: (" << vector.x << ", " << vector.y << ", " << vector.z << ")";
    return stream;
}
