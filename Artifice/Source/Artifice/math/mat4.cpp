#include "mat4.h"

#include <math.h>
#include <iomanip>

#include "math_functions.h"


mat4::mat4()
: elements{}
{

}

mat4::mat4(float diagonal)
: elements{}
{

    elements[0 + 0 * 4] = diagonal;
    elements[1 + 1 * 4] = diagonal;
    elements[2 + 2 * 4] = diagonal;
    elements[3 + 3 * 4] = diagonal;
}

mat4 mat4::identity() {
    return mat4(1.0f);
}

mat4& mat4::multiply(const mat4& other)
{
    float result[4 * 4];
    for (unsigned int y = 0; y < 4; y++)
        for (unsigned int x = 0; x < 4; x++)
        {
            float sum = 0.0f;
            for (unsigned int i = 0; i < 4; i++)
                sum += elements[x + i * 4] * other.elements[i + y * 4];
            result[x + y * 4] = sum;
        }
    memcpy(elements, result, sizeof(float) * 4 * 4);
    return *this;
}

vec3 mat4::multiply(const vec3& other) const
{
    return other.multiply(*this);
}

vec4 mat4::multiply(const vec4& other) const
{
    return other.multiply(*this);
}

mat4 operator*(mat4 left, const mat4& right)
{
    return left.multiply(right);
}

mat4& mat4::operator*=(const mat4 &other)
{
    return multiply(other);
}

vec3 operator*(const mat4& left, const vec3& right)
{
    return left.multiply(right);
}

vec4 operator*(const mat4& left, const vec4& right)
{
    return left.multiply(right);
}

mat4 mat4::inverse() const
{
    float temp[16];

    temp[0] = elements[5] * elements[10] * elements[15] -
        elements[5] * elements[11] * elements[14] -
        elements[9] * elements[6] * elements[15] +
        elements[9] * elements[7] * elements[14] +
        elements[13] * elements[6] * elements[11] -
        elements[13] * elements[7] * elements[10];

    temp[4] = -elements[4] * elements[10] * elements[15] +
        elements[4] * elements[11] * elements[14] +
        elements[8] * elements[6] * elements[15] -
        elements[8] * elements[7] * elements[14] -
        elements[12] * elements[6] * elements[11] +
        elements[12] * elements[7] * elements[10];

    temp[8] = elements[4] * elements[9] * elements[15] -
        elements[4] * elements[11] * elements[13] -
        elements[8] * elements[5] * elements[15] +
        elements[8] * elements[7] * elements[13] +
        elements[12] * elements[5] * elements[11] -
        elements[12] * elements[7] * elements[9];

    temp[12] = -elements[4] * elements[9] * elements[14] +
        elements[4] * elements[10] * elements[13] +
        elements[8] * elements[5] * elements[14] -
        elements[8] * elements[6] * elements[13] -
        elements[12] * elements[5] * elements[10] +
        elements[12] * elements[6] * elements[9];

    temp[1] = -elements[1] * elements[10] * elements[15] +
        elements[1] * elements[11] * elements[14] +
        elements[9] * elements[2] * elements[15] -
        elements[9] * elements[3] * elements[14] -
        elements[13] * elements[2] * elements[11] +
        elements[13] * elements[3] * elements[10];

    temp[5] = elements[0] * elements[10] * elements[15] -
        elements[0] * elements[11] * elements[14] -
        elements[8] * elements[2] * elements[15] +
        elements[8] * elements[3] * elements[14] +
        elements[12] * elements[2] * elements[11] -
        elements[12] * elements[3] * elements[10];

    temp[9] = -elements[0] * elements[9] * elements[15] +
        elements[0] * elements[11] * elements[13] +
        elements[8] * elements[1] * elements[15] -
        elements[8] * elements[3] * elements[13] -
        elements[12] * elements[1] * elements[11] +
        elements[12] * elements[3] * elements[9];

    temp[13] = elements[0] * elements[9] * elements[14] -
        elements[0] * elements[10] * elements[13] -
        elements[8] * elements[1] * elements[14] +
        elements[8] * elements[2] * elements[13] +
        elements[12] * elements[1] * elements[10] -
        elements[12] * elements[2] * elements[9];

    temp[2] = elements[1] * elements[6] * elements[15] -
        elements[1] * elements[7] * elements[14] -
        elements[5] * elements[2] * elements[15] +
        elements[5] * elements[3] * elements[14] +
        elements[13] * elements[2] * elements[7] -
        elements[13] * elements[3] * elements[6];

    temp[6] = -elements[0] * elements[6] * elements[15] +
        elements[0] * elements[7] * elements[14] +
        elements[4] * elements[2] * elements[15] -
        elements[4] * elements[3] * elements[14] -
        elements[12] * elements[2] * elements[7] +
        elements[12] * elements[3] * elements[6];

    temp[10] = elements[0] * elements[5] * elements[15] -
        elements[0] * elements[7] * elements[13] -
        elements[4] * elements[1] * elements[15] +
        elements[4] * elements[3] * elements[13] +
        elements[12] * elements[1] * elements[7] -
        elements[12] * elements[3] * elements[5];

    temp[14] = -elements[0] * elements[5] * elements[14] +
        elements[0] * elements[6] * elements[13] +
        elements[4] * elements[1] * elements[14] -
        elements[4] * elements[2] * elements[13] -
        elements[12] * elements[1] * elements[6] +
        elements[12] * elements[2] * elements[5];

    temp[3] = -elements[1] * elements[6] * elements[11] +
        elements[1] * elements[7] * elements[10] +
        elements[5] * elements[2] * elements[11] -
        elements[5] * elements[3] * elements[10] -
        elements[9] * elements[2] * elements[7] +
        elements[9] * elements[3] * elements[6];

    temp[7] = elements[0] * elements[6] * elements[11] -
        elements[0] * elements[7] * elements[10] -
        elements[4] * elements[2] * elements[11] +
        elements[4] * elements[3] * elements[10] +
        elements[8] * elements[2] * elements[7] -
        elements[8] * elements[3] * elements[6];

    temp[11] = -elements[0] * elements[5] * elements[11] +
        elements[0] * elements[7] * elements[9] +
        elements[4] * elements[1] * elements[11] -
        elements[4] * elements[3] * elements[9] -
        elements[8] * elements[1] * elements[7] +
        elements[8] * elements[3] * elements[5];

    temp[15] = elements[0] * elements[5] * elements[10] -
        elements[0] * elements[6] * elements[9] -
        elements[4] * elements[1] * elements[10] +
        elements[4] * elements[2] * elements[9] +
        elements[8] * elements[1] * elements[6] -
        elements[8] * elements[2] * elements[5];

    float determinant = elements[0] * temp[0] + elements[1] * temp[4] + elements[2] * temp[8] + elements[3] * temp[12];
    determinant = 1.0f / determinant;

    mat4 result;

    for (unsigned int i = 0; i < 4 * 4; i++)
        result.elements[i] = temp[i] * determinant;

    return result;
}

mat4 mat4::transpose() const
{
    mat4 result;
    for (unsigned int col = 0; col < 4; col++)
    {
        for(unsigned int row = 0; row < 4; row++)
        {
            result.get(col, row) = elements[row + col * 4];
        }
    }
    
    return result;
}

void mat4::decompose(const mat4& transform, vec3& translation, vec3& scale, mat4& rotation)
{
    mat4 m = transform;

    vec4& last_column = m.columns[3];

    translation = vec3(last_column.x, last_column.y, last_column.z);
    last_column = vec4(0.0f);
    
    scale = vec3(m.columns[0].Magnitude(), m.columns[1].Magnitude(), m.columns[2].Magnitude());

    m.columns[0] /= scale.x;
    m.columns[1] /= scale.y;
    m.columns[2] /= scale.z;

    rotation = m;
}

mat4 mat4::orthographic(float left, float right, float bottom, float top, float near, float far)
{
    mat4 result(1.0f);

    result.elements[0 + 0 * 4] =  2.0f / (right - left);
    result.elements[1 + 1 * 4] =  2.0f / (bottom - top);
    result.elements[2 + 2 * 4] = -1.0f / (near - far);

    result.elements[0 + 3 * 4] = -(right + left) / (right - left);
    result.elements[1 + 3 * 4] = -(bottom + top) / (bottom - top);
    result.elements[2 + 3 * 4] =  near / (near - far);

    return result;
}

mat4 mat4::perspective(float aspectRatio, float fov, float near, float far) {
    mat4 result;
    float q = 1 / tan(math::toRadians(fov * 0.5f));

    result.elements[0 + 0 * 4] =  q / aspectRatio;
    result.elements[1 + 1 * 4] = -q;
    result.elements[2 + 2 * 4] =  far / (near - far);
    result.elements[3 + 2 * 4] =  -1.0f;
    result.elements[2 + 3 * 4] =  -(far * near) / (far - near);
#if 0

    float q = cos(fov * 0.5) / sin(fov * 0.5);
    result.elements[0 + 0 * 4] =  q / aspectRatio;
    result.elements[1 + 1 * 4] = -q;
    result.elements[2 + 2 * 4] =  (far + near) / (near - far);
    result.elements[3 + 2 * 4] =  -1.0f;
    result.elements[2 + 3 * 4] =  2 * far * near / (near - far);
#endif

    return result;
}

mat4 mat4::translation(const vec3& translation) {
    mat4 result(1.0f);

    result.elements[0 + 3 * 4] = translation.x;
    result.elements[1 + 3 * 4] = translation.y;
    result.elements[2 + 3 * 4] = translation.z;

    return result;
}

mat4 mat4::rotation(float angle, const vec3 &axis) {
    mat4 result(1.0f);

    float c = cos(math::toRadians(angle));
    float s = sin(math::toRadians(angle));
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    result.elements[0 + 0 * 4] = x * x * (1 - c) + c;
    result.elements[1 + 0 * 4] = x * y * (1 - c) + z * s;
    result.elements[2 + 0 * 4] = x * z * (1 - c) - y * s;

    result.elements[0 + 1 * 4] = x * y * (1 - c) - z * s;
    result.elements[1 + 1 * 4] = y * y * (1 - c) + c;
    result.elements[2 + 1 * 4] = y * z * (1 - c) + x * s;

    result.elements[0 + 2 * 4] = x * z * (1 - c) + y * s;
    result.elements[1 + 2 * 4] = y * z * (1 - c) - x * s;
    result.elements[2 + 2 * 4] = z * z * (1 - c) + c;

    return result;
}

mat4 mat4::rotation(const quat& rot)
{
    mat4 result = identity();

    float x = rot.x;
    float y = rot.y;
    float z = rot.z;
    float w = rot.w;
    
    result.get(0, 0) = 1.0f - 2.0f * (y * y + z * z);
    result.get(1, 0) = 2.0f * (x * y + z * w);
    result.get(2, 0) = 2.0f * (x * z - y * w);

    result.get(0, 1) = 2.0f * (x * y - z * w);
    result.get(1, 1) = 1.0f - 2.0f * (x * x + z * z);
    result.get(2, 1) = 2.0f * (y * z - x * w);

    result.get(0, 2) = 2.0f * (x * z - y * w);
    result.get(1, 2) = 2.0f * (y * z - x * w);
    result.get(2, 2) = 1.0f - 2.0f * (x * x + y * y);

    return result;
}

mat4 mat4::scale(const vec3 &scale) {
    mat4 result(1.0f);

    result.elements[0 + 0 * 4] = scale.x;
    result.elements[1 + 1 * 4] = scale.y;
    result.elements[2 + 2 * 4] = scale.z;

    return result;
}

std::ostream& operator<<(std::ostream& stream, const mat4& matrix)
{
    stream << std::setprecision(5) << std::fixed;
    stream << "mat4: " << std::endl;
    for (int x = 0; x < 4; x++)
    {
        stream << std::endl;
        for (int y = 0; y < 4; y++)
        {
            if(y == 0)
                stream << "|  ";
            if(matrix.elements[x + y * 4] < 0)
                stream << matrix.elements[x + y * 4] << "  ";
            else
                stream << " " << matrix.elements[x + y * 4] << "  ";
            if(y == 3)
                stream << "|";
        }
    }
    return stream;
}
