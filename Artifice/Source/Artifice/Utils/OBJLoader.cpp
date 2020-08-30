#include "OBJLoader.h"

#include <iostream>

#if 0
#include <meshoptimizer.h>
#include <tiny_obj_loader.h>



ModelData OBJLoader::LoadOBJ(const std::string& model_path)
{
    ModelData data;

    std::vector<Vertex> pre_opt_vertices;
    std::vector<unsigned int> pre_opt_indices;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str());
    AR_CORE_ASSERT(success, "%s %s", warn.c_str(), err.c_str());

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex;

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            vertex.uv = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            pre_opt_vertices.push_back(vertex);
            pre_opt_indices.push_back((unsigned int)data.indices.size());
        }
    }

    std::cout << "PRE-OPT | Vertex count: " << pre_opt_vertices.size() << ", Index count: " << pre_opt_indices.size() << std::endl;

    size_t index_count = pre_opt_indices.size();
    std::vector<unsigned int> remap(index_count); // allocate temporary memory for the remap table
    size_t vertex_count = meshopt_generateVertexRemap(&remap[0], NULL, index_count, &pre_opt_vertices[0], index_count, sizeof(Vertex));

    std::vector<unsigned int> indices(index_count);
    std::vector<Vertex> vertices(vertex_count);

    meshopt_remapIndexBuffer(indices.data(), NULL, index_count, &remap[0]);
    meshopt_remapVertexBuffer(vertices.data(), &pre_opt_vertices[0], index_count, sizeof(Vertex), &remap[0]);
    meshopt_optimizeVertexCache(indices.data(), indices.data(), index_count, vertex_count);
    meshopt_optimizeOverdraw(indices.data(), indices.data(), index_count, (float*)vertices.data(), vertex_count, sizeof(Vertex), 1.05f);
    meshopt_optimizeVertexFetch(vertices.data(), indices.data(), index_count, vertices.data(), vertex_count, sizeof(Vertex));


    std::cout << "POST-OPT | Vertex count: " << vertices.size() << ", Index count: " << indices.size() << std::endl;

    for (unsigned int i = 0; i < vertex_count; i++)
    {
        data.positions.push_back(vertices[i].pos);
        data.colors.push_back(vertices[i].color);
        data.uvs.push_back(vertices[i].uv);
    }
    data.indices = indices;

    return data;
}
#endif