#include "Sphere.h"


Sphere::Sphere()
: center(vec3()), radius(0.0f)
{
    
}

Sphere::Sphere(const vec3& center, float radius)
: center(center), radius(radius)
{
    
}

Sphere::Sphere(const AABB& box) //creates bounding sphere
: center(box.GetCenter()), radius(box.Max.Distance(box.Min) * 0.5)
{
    
}
