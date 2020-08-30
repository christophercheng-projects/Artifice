#include "AABB.h"


AABB::AABB()
: Min(vec3()), Max(vec3())
{
    
}

AABB::AABB(const vec3& min, const vec3& max)
: Min(min), Max(max)
{
    
}


AABB::AABB(const std::vector<vec3>& positions)
: Min(vec3()), Max(vec3())
{
    for (unsigned int i = 0; i < positions.size(); i++)
    {
        const vec3& p = positions[i];
        
        if (p.x < Min.x) Min.x = p.x;
        if (p.y < Min.y) Min.y = p.y;
        if (p.z < Min.z) Min.z = p.z;
        
        if (p.x > Max.x) Max.x = p.x;
        if (p.y > Max.y) Max.y = p.y;
        if (p.z > Max.z) Max.z = p.z;
    }
}

vec3 AABB::GetCenter() const
{
    return (Max + Min) * 0.5;
}
