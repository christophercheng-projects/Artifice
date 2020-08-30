#pragma once

#include <unordered_map>
#include <vector>


#include "Artifice/Core/Log.h"

#include "Artifice/math/vec2.h"
#include "Artifice/math/vec3.h"

struct Vertex
{
    vec3 pos;
    vec3 color;
    vec2 uv;
};

struct PreOptimizedData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct ModelData
{
    std::vector<vec3> positions;
    std::vector<vec3> colors;
    std::vector<vec2> uvs;

    std::vector<unsigned int> indices;
};

class OBJLoader
{
public:
    static ModelData LoadOBJ(const std::string& model_path);
};
