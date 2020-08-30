#include "vec4.h"

#include "vec3.h"
#include "mat4.h"
#include "math_functions.h"


vec4::vec4()
: x(0.0f), y(0.0f), z(0.0f), w(0.0f)
{
    
}
vec4::vec4(float v)
: x(v), y(v), z(v), w(v)
{
    
}

vec4::vec4(float x, float y, float z, float w)
: x(x), y(y), z(z), w(w)
{
    
}

vec4::vec4(const vec3& xyz, float w)
: x(xyz.x), y(xyz.y), z(xyz.z), w(w)
{

}

vec4& vec4::add(const vec4& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    
    return *this;
}

vec4& vec4::subtract(const vec4& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    
    return *this;
}

vec4& vec4::multiply(const vec4& other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    
    return *this;
}

vec4& vec4::divide(const vec4& other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    
    return *this;
}

vec4 vec4::multiply(const mat4& transform) const
{
    return vec4(
                transform.elements[0 + 0 * 4] * x +
                transform.elements[0 + 1 * 4] * y +
                transform.elements[0 + 2 * 4] * z +
                transform.elements[0 + 3 * 4] * w,
                
                transform.elements[1 + 0 * 4] * x +
                transform.elements[1 + 1 * 4] * y +
                transform.elements[1 + 2 * 4] * z +
                transform.elements[1 + 3 * 4] * w,
                
                transform.elements[2 + 0 * 4] * x +
                transform.elements[2 + 1 * 4] * y +
                transform.elements[2 + 2 * 4] * z +
                transform.elements[2 + 3 * 4] * w,
                
                transform.elements[3 + 0 * 4] * x +
                transform.elements[3 + 1 * 4] * y +
                transform.elements[3 + 2 * 4] * z +
                transform.elements[3 + 3 * 4] * w
                );
}


vec4 operator+(vec4 left, const vec4& right)
{
    return left.add(right);
}

vec4 operator-(vec4 left, const vec4& right)
{
    return left.subtract(right);
}

vec4 operator*(vec4 left, const vec4& right)
{
    return left.multiply(right);
}

vec4 operator/(vec4 left, const vec4& right)
{
    return left.divide(right);
}

vec4& vec4::operator+=(const vec4& other)
{
    return add(other);
}

vec4& vec4::operator-=(const vec4& other)
{
    return subtract(other);
}

vec4& vec4::operator*=(const vec4& other)
{
    return multiply(other);
}

vec4& vec4::operator/=(const vec4& other)
{
    return divide(other);
}

bool vec4::operator==(const vec4& other)
{
    return (x == other.x && y == other.y && z == other.z && w == other.w);
}

bool vec4::operator!=(const vec4& other)
{
    return !(*this == other);
}

float vec4::Magnitude() const
{
    return math::sqrt(x * x + y * y + z * z + w * w);
}

vec4 vec4::Normalize() const
{
    float length = Magnitude();
    return vec4(x / length, y / length, z / length, w / length);
}

std::ostream& operator<<(std::ostream& stream, const vec4& vector)
{
    stream << "vec4: (" << vector.x << ", " << vector.y << ", "<< vector.z << ", " << vector.w << ")";
    return stream;
}
