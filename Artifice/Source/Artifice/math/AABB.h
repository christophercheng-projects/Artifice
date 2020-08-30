#pragma once

#include <vector>

#include "vec3.h"


struct AABB
{
    vec3 Min;
    vec3 Max;
    
    AABB();
    AABB(const vec3& min, const vec3& max);
    AABB(const std::vector<vec3>& positions);
    
    vec3 GetCenter() const;
};
