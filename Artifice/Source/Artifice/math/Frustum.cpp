#include "Frustum.h"


void Frustum::ExtractPlanes(const mat4& m)
{
    planes[LEFT].x   = m.elements[0 + 3 * 4] + m.elements[0 + 0 * 4];
    planes[LEFT].y   = m.elements[1 + 3 * 4] + m.elements[1 + 0 * 4];
    planes[LEFT].z   = m.elements[2 + 3 * 4] + m.elements[2 + 0 * 4];
    planes[LEFT].w   = m.elements[3 + 3 * 4] + m.elements[3 + 0 * 4];
    
    planes[RIGHT].x  = m.elements[0 + 3 * 4] - m.elements[0 + 0 * 4];
    planes[RIGHT].y  = m.elements[1 + 3 * 4] - m.elements[1 + 0 * 4];
    planes[RIGHT].z  = m.elements[2 + 3 * 4] - m.elements[2 + 0 * 4];
    planes[RIGHT].w  = m.elements[3 + 3 * 4] - m.elements[3 + 0 * 4];
    
    planes[TOP].x    = m.elements[0 + 3 * 4] - m.elements[0 + 1 * 4];
    planes[TOP].y    = m.elements[1 + 3 * 4] - m.elements[1 + 1 * 4];
    planes[TOP].z    = m.elements[2 + 3 * 4] - m.elements[2 + 1 * 4];
    planes[TOP].w    = m.elements[3 + 3 * 4] - m.elements[3 + 1 * 4];

    planes[BOTTOM].x = m.elements[0 + 3 * 4] + m.elements[0 + 1 * 4];
    planes[BOTTOM].y = m.elements[1 + 3 * 4] + m.elements[1 + 1 * 4];
    planes[BOTTOM].z = m.elements[2 + 3 * 4] + m.elements[2 + 1 * 4];
    planes[BOTTOM].w = m.elements[3 + 3 * 4] + m.elements[3 + 1 * 4];
    
    planes[NEAR].x   = m.elements[0 + 3 * 4] + m.elements[0 + 2 * 4];
    planes[NEAR].y   = m.elements[1 + 3 * 4] + m.elements[1 + 2 * 4];
    planes[NEAR].z   = m.elements[2 + 3 * 4] + m.elements[2 + 2 * 4];
    planes[NEAR].w   = m.elements[3 + 3 * 4] + m.elements[3 + 2 * 4];
    
    planes[FAR].x    = m.elements[0 + 3 * 4] - m.elements[0 + 2 * 4];
    planes[FAR].y    = m.elements[1 + 3 * 4] - m.elements[1 + 2 * 4];
    planes[FAR].z    = m.elements[2 + 3 * 4] - m.elements[2 + 2 * 4];
    planes[FAR].w    = m.elements[3 + 3 * 4] - m.elements[3 + 2 * 4];
    
    for (unsigned int i = 0; i < 6; i++)
    {
        planes[i].Normalize();
    }
}

bool Frustum::IsSphereVisible(const Sphere& sphere)
{
    for (unsigned int i = 0; i < 6; i++)
    {
        if (planes[i].x * sphere.center.x + planes[i].y * sphere.center.y + planes[i].z * sphere.center.z + planes[i].w + sphere.radius <= 0)
        {
            return false;
        }
    }
    
    return true;
}
