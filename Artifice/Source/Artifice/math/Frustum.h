#pragma once

#include <vector>

#include "mat4.h"
#include "Sphere.h"
#include "vec4.h"


struct Frustum
{
    enum Side 
    {
        LEFT   = 0,
        RIGHT  = 1,
        TOP    = 2,
        BOTTOM = 3,
        NEAR   = 4,
        FAR    = 5
    };
    
    vec4 planes[6];
    
    void ExtractPlanes(const mat4& m);
    
    bool IsSphereVisible(const Sphere& sphere);
};
