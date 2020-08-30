#pragma once

#include <unordered_map>


#include "Artifice/Core/Timestep.h"
#include "Artifice/Utils/Timer.h"

#include "Artifice/math/math.h"

#include "Artifice/Graphics/Material.h"
#include "Artifice/Graphics/Shader.h"
#include "Artifice/Graphics/RenderHandle.h"

#include "Artifice/Graphics/Device.h"

struct aiAnimation;
struct aiNode;
struct aiNodeAnim;
struct aiScene;
struct aiMaterial;
namespace Assimp { class Importer; }


struct StaticVertex
{
    vec3 Position;
    vec3 Normal;
    vec3 Tangent;
    vec3 Binormal;
    vec2 TexCoord;
};

struct AnimatedVertex
{
    vec3 Position;
    vec3 Normal;
    vec3 Tangent;
    vec3 Binormal;
    vec2 TexCoord;

    int32 BoneID[4]{0, 0, 0, 0};
    float BoneWeight[4]{0.0f, 0.0f, 0.0f, 0.0f};

    void AddBone(int32 bone_index, float bone_weight)
    {
        for (int32 i = 0; i < 4; i++)
        {
            if (BoneWeight[i] == 0.0f)
            {
                BoneID[i] = bone_index;
                BoneWeight[i] = bone_weight;
                return;
            }
        }

        AR_CORE_WARN("StaticVertex can only have upto 4 bones. Bone discarded (ID = %i, Weight = %f", bone_index, bone_weight);
    }
};

struct BoneInfo
{
    AABB BoundingBox;
    mat4 FinalTransform = mat4(1.0f);
    mat4 Offset = mat4(1.0f);
};

struct Index
{
    uint32 V0, V1, V2;
};

struct Triangle
{
    vec3 V0, V1, V2;

    Triangle(vec3 v0, vec3 v1, vec3 v2)
        : V0(v0), V1(v1), V2(v2) {}
};

struct Submesh
{
    uint32 BaseVertex;
    uint32 BaseIndex;
    uint32 IndexCount;
    uint32 MaterialIndex;

    mat4 Transform = mat4(1.0f);
    AABB BoundingBox;

    Submesh(uint32 baseVertex, uint32 baseIndex, uint32 indexCount, uint32 materialIndex)
        : BaseVertex(baseVertex), BaseIndex(baseIndex), IndexCount(indexCount), MaterialIndex(materialIndex) {}
};

class Mesh
{
public:
    Mesh(const std::string& filepath);
    ~Mesh();

    void Bind(CommandBuffer* cmd);
    void OnUpdate(Timestep ts);

    std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }
    const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }
    const std::vector<Triangle>& GetTriangleCache(uint32 index) { return m_TriangleCache[index]; }

    const std::string& GetFilePath() const { return m_FilePath; }
    const std::vector<RenderHandle>& GetTextures() const { return m_Textures; }
    uint32 GetIndexCount() {return m_Indices.size();}

    const std::vector<PBRMaterialSystem*>& GetMaterialSystems() const { return m_MaterialSystems; }

    uint32 GetBoneCount() const { return m_BoneCount; }
    const mat4& GetBoneTransform(uint32 index) const { return m_BoneTransforms[index]; };
    const std::vector<mat4>& GetBoneTransforms() const { return m_BoneTransforms; };

    const std::vector<BoneInfo>& GetBoneInfo() const { return m_BoneInfo; }

    bool IsAnimated() { return m_IsAnimated; }
    bool& IsAnimationPlaying() { return m_AnimationPlaying; }
    const bool IsAnimationPlaying() const { return m_AnimationPlaying; }

    float& AnimationTimeMultiplier() { return m_TimeMultiplier; }
    const float AnimationTimeMultiplier() const { return m_TimeMultiplier; }

private:
    std::string m_FilePath;

    VertexLayout m_VertexLayout;

    Assimp::Importer* m_Importer = nullptr;
    const aiScene* m_Scene;

    std::vector<Submesh> m_Submeshes;
    std::unordered_map<uint32, std::vector<Triangle>> m_TriangleCache;

    std::vector<AnimatedVertex> m_AnimatedVertices;
    std::vector<StaticVertex> m_StaticVertices;
    std::vector<Index> m_Indices;
    RenderHandle m_VertexBuffer;
    RenderHandle m_IndexBuffer;

    mat4 m_InverseRootTransform;

    uint32 m_BoneCount = 0;
    std::vector<BoneInfo> m_BoneInfo;
    std::unordered_map<std::string, uint32> m_BoneMap;
    std::vector<mat4> m_BoneTransforms;

    bool m_AnimationPlaying = true;
    float m_AnimationTime = 0.0f;
    bool m_IsAnimated = false;
    float m_TimeMultiplier = 1.0f;

    std::vector<PBRMaterialSystem*> m_MaterialSystems;

    std::vector<RenderHandle> m_Textures;
    std::vector<RenderHandle> m_NormalMaps;

    std::vector<RenderHandle> m_OwnedTextures;
    std::vector<RenderHandle> m_OwnedSamplers;

    inline static const uint32 s_MaxBones = 100;

    void TraverseNodes(aiNode* node, const mat4& parentTransform = mat4(1.0f));

    void UpdateBones(float time);
    void TraverseNodeHierarchy(float time, const aiNode* node, const mat4& parentTransform = mat4(1.0f));
    const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName);

    uint32_t FindPosition(float time, const aiNodeAnim* nodeAnim);
    uint32_t FindRotation(float time, const aiNodeAnim* nodeAnim);
    uint32_t FindScaling(float time, const aiNodeAnim* nodeAnim);

    vec3 InterpolateTranslation(float time, const aiNodeAnim* nodeAnim);
    quat InterpolateRotation(float time, const aiNodeAnim* nodeAnim);
    vec3 InterpolateScale(float time, const aiNodeAnim* nodeAnim);
};
