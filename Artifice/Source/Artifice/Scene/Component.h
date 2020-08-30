#pragma once

#include <string>

#include "Artifice/Core/Core.h"
#include "Artifice/Graphics/Camera.h"
#include "Artifice/Graphics/Mesh.h"
#include "Artifice/math/math.h"

struct TagComponent
{
    std::string Tag;
};

struct TransformComponent
{
    mat4 Transform = mat4(1.0f);
};

struct CameraComponent
{
    PerspectiveCamera Camera;
    bool Primary = true;
};

struct MeshComponent
{
    Mesh* Mesh;
};

struct BoxCollider
{
    AABB BoundingBox;
};