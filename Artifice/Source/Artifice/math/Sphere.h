#pragma once

#include "AABB.h"
#include "mat4.h"
#include "vec3.h"


struct Sphere
{
    vec3 center;
    float radius;
    
    Sphere();
    Sphere(const vec3& center, float radius);
    Sphere(const AABB& box); //creates bounding sphere
};
