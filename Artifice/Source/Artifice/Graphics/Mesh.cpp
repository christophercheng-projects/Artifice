#include "Mesh.h"

#include <filesystem>

#include "Artifice/Core/Log.h"

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Artifice/Core/Application.h"
#include "Artifice/Graphics/Resources.h"
#include "Artifice/Graphics/Renderer.h"
#include "Artifice/Utils/FileUtils.h"


#define MESH_DEBUG_LOG 1
#if MESH_DEBUG_LOG
#define AR_MESH_LOG(...) AR_CORE_TRACE(__VA_ARGS__)
#else
#define AR_MESH_LOG(...)
#endif

static const uint32 s_MeshImportFlags =
    aiProcess_CalcTangentSpace |
    aiProcess_Triangulate |
    aiProcess_GenNormals |
    aiProcess_SplitLargeMeshes |
    aiProcess_ValidateDataStructure |
    aiProcess_SortByPType |
    aiProcess_GenUVCoords |
    aiProcess_OptimizeMeshes;

mat4 AssimpMat4ToMat4(const aiMatrix4x4& matrix)
{
    mat4 result;

    // a - d : Rows; 1 - 4: Columns

    result.get(0, 0) = matrix.a1;
    result.get(1, 0) = matrix.b1;
    result.get(2, 0) = matrix.c1;
    result.get(3, 0) = matrix.d1;
    result.get(0, 1) = matrix.a2;
    result.get(1, 1) = matrix.b2;
    result.get(2, 1) = matrix.c2;
    result.get(3, 1) = matrix.d2;
    result.get(0, 2) = matrix.a3;
    result.get(1, 2) = matrix.b3;
    result.get(2, 2) = matrix.c3;
    result.get(3, 2) = matrix.d3;
    result.get(0, 3) = matrix.a4;
    result.get(1, 3) = matrix.b4;
    result.get(2, 3) = matrix.c4;
    result.get(3, 3) = matrix.d4;

    return result;
}

struct LogStream : public Assimp::LogStream
{
    static void Initialize()
    {
        if (Assimp::DefaultLogger::isNullLogger())
        {
            Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
            Assimp::DefaultLogger::get()->attachStream(new LogStream, Assimp::Logger::Err | Assimp::Logger::Warn);
        }
    }

    void write(const char* message) override
    {
        AR_CORE_ERROR("%s", message);
    }
};

static std::string GetTexturePath(const std::string& curPath, const std::string& texturePath)
{
    std::filesystem::path path(curPath);
    auto parentPath = path.parent_path();
    parentPath /= std::string(texturePath);
    return parentPath.string();
}

static std::string GetTexturePath(const std::string& curPath, const aiString& texturePath)
{
    return GetTexturePath(curPath, texturePath.data);
}

Mesh::Mesh(const std::string& filepath)
    : m_FilePath(filepath)
{
    LogStream::Initialize();

    AR_CORE_TRACE("Loading mesh from %s", filepath.c_str());

    m_Importer = new Assimp::Importer();
    m_Scene = m_Importer->ReadFile(filepath, s_MeshImportFlags);

    if (!m_Scene || !m_Scene->HasMeshes())
    {
        AR_CORE_ASSERT(false, "Failed to load mesh!");
        return;
    }

    m_IsAnimated = m_Scene->HasAnimations();
    m_InverseRootTransform = AssimpMat4ToMat4(m_Scene->mRootNode->mTransformation).inverse();

    uint32_t vertexCount = 0, indexCount = 0;
    uint32_t numMeshes = m_Scene->mNumMeshes;
    m_Submeshes.reserve(numMeshes);
    for (uint32_t m = 0; m < numMeshes; m++)
    {
        aiMesh* mesh = m_Scene->mMeshes[m];
        Submesh& submesh = m_Submeshes.emplace_back(vertexCount, indexCount, mesh->mNumFaces * 3, mesh->mMaterialIndex);
        auto& aabb = submesh.BoundingBox;

        vertexCount += mesh->mNumVertices;
        indexCount += submesh.IndexCount;

        if (m_IsAnimated)
        {
            for (uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                AnimatedVertex vertex;
                vertex.Position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
                vertex.Normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};

                aabb.Min.x = math::min(vertex.Position.x, aabb.Min.x);
                aabb.Min.y = math::min(vertex.Position.y, aabb.Min.y);
                aabb.Min.z = math::min(vertex.Position.z, aabb.Min.z);
                aabb.Max.x = math::max(vertex.Position.x, aabb.Max.x);
                aabb.Max.y = math::max(vertex.Position.y, aabb.Max.y);
                aabb.Max.z = math::max(vertex.Position.z, aabb.Max.z);

                if (mesh->HasTangentsAndBitangents())
                {
                    vertex.Tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
                    vertex.Binormal = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
                }

                if (mesh->HasTextureCoords(0))
                    vertex.TexCoord = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};

                m_AnimatedVertices.push_back(vertex);
            }

            for (uint32_t i = 0; i < mesh->mNumBones; i++)
            {
                aiBone* bone = mesh->mBones[i];
                if (bone->mNumWeights == 0)
                    continue;

                std::string boneName = bone->mName.data;
                uint32_t boneIndex = 0;
                if (m_BoneMap.find(boneName) == m_BoneMap.end())
                {
                    BoneInfo boneInfo;
                    boneInfo.Offset = AssimpMat4ToMat4(bone->mOffsetMatrix);

                    m_BoneInfo.push_back(boneInfo);
                    boneIndex = m_BoneCount++;
                    m_BoneMap[boneName] = boneIndex;
                }
                else
                {
                    boneIndex = m_BoneMap[boneName];
                }

                for (uint32_t j = 0; j < bone->mNumWeights; j++)
                {
                    uint32_t vertexID = submesh.BaseVertex + bone->mWeights[j].mVertexId;
                    float weight = bone->mWeights[j].mWeight;

                    auto& aabb = m_BoneInfo[boneIndex].BoundingBox;
                    auto& vertex = m_AnimatedVertices[vertexID];
                    aabb.Min.x = math::min(vertex.Position.x, aabb.Min.x);
                    aabb.Min.y = math::min(vertex.Position.y, aabb.Min.y);
                    aabb.Min.z = math::min(vertex.Position.z, aabb.Min.z);
                    aabb.Max.x = math::max(vertex.Position.x, aabb.Max.x);
                    aabb.Max.y = math::max(vertex.Position.y, aabb.Max.y);
                    aabb.Max.z = math::max(vertex.Position.z, aabb.Max.z);

                    vertex.AddBone(boneIndex, weight);
                }
            }

            m_BoneTransforms.resize(s_MaxBones);
            UpdateBones(0.0f);
        }
        else
        {
            for (uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                StaticVertex vertex;
                vertex.Position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
                vertex.Normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};

                aabb.Min.x = math::min(vertex.Position.x, aabb.Min.x);
                aabb.Min.y = math::min(vertex.Position.y, aabb.Min.y);
                aabb.Min.z = math::min(vertex.Position.z, aabb.Min.z);
                aabb.Max.x = math::max(vertex.Position.x, aabb.Max.x);
                aabb.Max.y = math::max(vertex.Position.y, aabb.Max.y);
                aabb.Max.z = math::max(vertex.Position.z, aabb.Max.z);

                if (mesh->HasTangentsAndBitangents())
                {
                    vertex.Tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
                    vertex.Binormal = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
                }

                if (mesh->HasTextureCoords(0))
                    vertex.TexCoord = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};

                m_StaticVertices.push_back(vertex);
            }
        }

        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            AR_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Faces must have 3 indices!");

            Index index = {mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2]};
            m_Indices.push_back(index);
            
            if (m_IsAnimated)
                m_TriangleCache[m].emplace_back(
                    m_AnimatedVertices[index.V0 + submesh.BaseVertex].Position,
                    m_AnimatedVertices[index.V1 + submesh.BaseVertex].Position,
                    m_AnimatedVertices[index.V2 + submesh.BaseVertex].Position);
            else
                m_TriangleCache[m].emplace_back(
                    m_StaticVertices[index.V0 + submesh.BaseVertex].Position,
                    m_StaticVertices[index.V1 + submesh.BaseVertex].Position,
                    m_StaticVertices[index.V2 + submesh.BaseVertex].Position);
        }

    }

    TraverseNodes(m_Scene->mRootNode);

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    CommandBuffer cmd = device->RequestCommandBuffer();
    cmd.Begin();

    BufferInfo vb;
    vb.Access = MemoryAccessType::Gpu;
    vb.BindFlags = RenderBindFlags::VertexBuffer | RenderBindFlags::TransferDestination;
    vb.Size = m_IsAnimated ? m_AnimatedVertices.size() * sizeof(AnimatedVertex) : m_StaticVertices.size() * sizeof(StaticVertex);

    BufferInfo ib;
    ib.Access = MemoryAccessType::Gpu;
    ib.BindFlags = RenderBindFlags::IndexBuffer | RenderBindFlags::TransferDestination;
    ib.Size = m_Indices.size() * sizeof(Index);

    m_VertexBuffer = device->CreateBuffer(vb);
    m_IndexBuffer = device->CreateBuffer(ib);

    cmd.UploadBufferData(m_VertexBuffer, vb.Size, m_IsAnimated ? (void*)m_AnimatedVertices.data() : (void*)m_StaticVertices.data(), ResourceState::VertexBuffer());
    cmd.UploadBufferData(m_IndexBuffer, ib.Size, m_Indices.data(), ResourceState::IndexBuffer());


    AR_MESH_LOG("------------------ Materials ------------------");

    uint32_t materialCount = m_Scene->mNumMaterials;
    m_Textures.resize(materialCount);
    m_MaterialSystems.resize(materialCount);
    for (uint32_t i = 0; i < materialCount; i++)
    {
        aiMaterial* material = m_Scene->mMaterials[i];

        m_MaterialSystems[i] = new PBRMaterialSystem(m_IsAnimated ? "PBR_Animated" : "PBR_Static");

        AR_MESH_LOG("  %s (Index = %u)", material->GetName().data, i);

        aiColor3D diffuseColor;
        float shininess, roughness, metalness;
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
        material->Get(AI_MATKEY_SHININESS, shininess);
        material->Get(AI_MATKEY_REFLECTIVITY, metalness);
        roughness = 1.0f - sqrt(shininess * 0.01f);

        AR_MESH_LOG("    COLOR     = %f, %f, %f", diffuseColor.r, diffuseColor.g, diffuseColor.b);
        AR_MESH_LOG("    ROUGHNESS = %f", roughness);
        AR_MESH_LOG("    METALNESS = %f", metalness);

        uint32_t textureCount = material->GetTextureCount(aiTextureType_DIFFUSE);
        AR_MESH_LOG("    %u diffuse textures found.", textureCount);

        textureCount = material->GetTextureCount(aiTextureType_SPECULAR);
        AR_MESH_LOG("    %u specular textures found.", textureCount);

        aiString texturePath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
        {
            std::string path = GetTexturePath(filepath, texturePath);
            AR_MESH_LOG("    Albedo map path = %s", path.c_str());

            ImageData img = ReadImage(path);
            TextureInfo info;
            info.Type = TextureType::Texture2D;
            info.BindFlags = RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination | RenderBindFlags::TransferSource;
            info.Width = img.Width;
            info.Height = img.Height;
            info.MipsEnabled = true;
            info.Format = VK_FORMAT_R8G8B8A8_SRGB;

            SamplerInfo samp;
            samp.Filter = VK_FILTER_LINEAR;
            samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samp.Mips = MIPS(img.Width, img.Height);
            RenderHandle sampler = device->CreateSampler(samp);
            m_OwnedSamplers.push_back(sampler);

            RenderHandle albedo = device->CreateTexture(info);
            m_OwnedTextures.push_back(albedo);
            cmd.UploadTextureData(albedo, img.Data.size(), img.Data.data(), true, ResourceState::SampledFragment());
            
            m_Textures[i] = albedo;
            
            m_MaterialSystems[i]->SetAlbedoTexture(albedo, sampler);
            m_MaterialSystems[i]->SetAlbedoTexToggle(1);
        }
        else
        {
            AR_MESH_LOG("    No albedo textures found.");

            m_MaterialSystems[i]->SetAlbedo({diffuseColor.r, diffuseColor.g, diffuseColor.b});
            m_MaterialSystems[i]->SetAlbedoTexToggle(0);
        }

        if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS)
        {
            std::string path = GetTexturePath(filepath, texturePath);
            AR_MESH_LOG("    Normal map path = %s", path.c_str());

            ImageData img = ReadImage(path);
            TextureInfo info;
            info.Type = TextureType::Texture2D;
            info.BindFlags = RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination | RenderBindFlags::TransferSource;
            info.Width = img.Width;
            info.Height = img.Height;
            info.MipsEnabled = true;
            info.Format = VK_FORMAT_R8G8B8A8_UNORM;

            SamplerInfo samp;
            samp.Filter = VK_FILTER_LINEAR;
            samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samp.Mips = MIPS(img.Width, img.Height);
            RenderHandle sampler = device->CreateSampler(samp);
            m_OwnedSamplers.push_back(sampler);

            RenderHandle normalMap = device->CreateTexture(info);
            m_OwnedTextures.push_back(normalMap);
            cmd.UploadTextureData(normalMap, img.Data.size(), img.Data.data(), true, ResourceState::SampledFragment());
            
            m_MaterialSystems[i]->SetNormalTexture(normalMap, sampler);
            m_MaterialSystems[i]->SetNormalTexToggle(1);
        }
        else
        {
            AR_MESH_LOG("    No normal textures found.");
            m_MaterialSystems[i]->SetNormalTexToggle(0);
        }

        if (material->GetTexture(aiTextureType_SHININESS, 0, &texturePath) == AI_SUCCESS)
        {
            std::string path = GetTexturePath(filepath, texturePath);
            AR_MESH_LOG("    Roughness(shininess) map path = %s", path.c_str());

            ImageData img = ReadImage(path);
            TextureInfo info;
            info.Type = TextureType::Texture2D;
            info.BindFlags = RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination | RenderBindFlags::TransferSource;
            info.Width = img.Width;
            info.Height = img.Height;
            info.MipsEnabled = true;
            info.Format = VK_FORMAT_R8G8B8A8_UNORM;

            SamplerInfo samp;
            samp.Filter = VK_FILTER_LINEAR;
            samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samp.Mips = MIPS(img.Width, img.Height);
            RenderHandle sampler = device->CreateSampler(samp);
            m_OwnedSamplers.push_back(sampler);

            RenderHandle roughnessMap = device->CreateTexture(info);
            m_OwnedTextures.push_back(roughnessMap);
            cmd.UploadTextureData(roughnessMap, img.Data.size(), img.Data.data(), true, ResourceState::SampledFragment());

            m_MaterialSystems[i]->SetRoughnessTexture(roughnessMap, sampler);
            m_MaterialSystems[i]->SetRoughnessTexToggle(1);
        }
        else
        {
            AR_MESH_LOG("    No roughness textures found.");

            m_MaterialSystems[i]->SetRoughness(roughness);
            m_MaterialSystems[i]->SetRoughnessTexToggle(0);
        }

        bool metalnessFound = false;
        for (uint32_t j = 0; j < material->mNumProperties; j++)
        {
            auto prop = material->mProperties[j];

            // TODO: Try to improve this
            if (prop->mType == aiPTI_String && std::string(prop->mKey.data) == "$raw.ReflectionFactor|file")
            {
                uint32_t strLength = *(uint32_t*)prop->mData;
                std::string texturePath(prop->mData + 4, strLength);

                std::string path = GetTexturePath(filepath, texturePath);
                AR_MESH_LOG("    Metalness(reflection factor) map path = %s", path.c_str());

                ImageData img = ReadImage(path);
                TextureInfo info;
                info.Type = TextureType::Texture2D;
                info.BindFlags = RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination | RenderBindFlags::TransferSource;
                info.Width = img.Width;
                info.Height = img.Height;
                info.MipsEnabled = true;
                info.Format = VK_FORMAT_R8G8B8A8_UNORM;

                SamplerInfo samp;
                samp.Filter = VK_FILTER_LINEAR;
                samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samp.Mips = MIPS(img.Width, img.Height);
                RenderHandle sampler = device->CreateSampler(samp);
                m_OwnedSamplers.push_back(sampler);

                RenderHandle metalnessMap = device->CreateTexture(info);
                m_OwnedTextures.push_back(metalnessMap);
                cmd.UploadTextureData(metalnessMap, img.Data.size(), img.Data.data(), true, ResourceState::SampledFragment());

                m_MaterialSystems[i]->SetMetalnessTexture(metalnessMap, sampler);
                m_MaterialSystems[i]->SetMetalnessTexToggle(1);

                metalnessFound = true;
                break;
            }
        }
        if (!metalnessFound)
        {
            AR_MESH_LOG("    No metalness textures found.");

            m_MaterialSystems[i]->SetMetalness(metalness);
            m_MaterialSystems[i]->SetMetalnessTexToggle(0);
        }

        AR_MESH_LOG("");
    }
    
    cmd.End();
    device->CommitCommandBuffer(cmd);
    device->SubmitQueue(QueueType::Universal, {}, {}, true);

    AR_MESH_LOG("-----------------------------------------------");
}

Mesh::~Mesh()
{
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    device->DestroyBuffer(m_VertexBuffer);
    device->DestroyBuffer(m_IndexBuffer);

    for (auto tex : m_OwnedTextures)
    {
        device->DestroyTexture(tex);
    }
    for (auto samp : m_OwnedSamplers)
    {
        device->DestroySampler(samp);
    }
    for (auto sys : m_MaterialSystems)
    {
        sys->Shutdown();
        delete sys;
    }
    m_MaterialSystems.clear();
}

void Mesh::Bind(CommandBuffer* cmd)
{
    cmd->BindVertexBuffers({m_VertexBuffer}, {0});
    cmd->BindIndexBuffer(m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::OnUpdate(Timestep ts)
{
    if (m_IsAnimated)
    {
        if (m_AnimationPlaying)
        {
            float ticksPerSecond = (float)(m_Scene->mAnimations[0]->mTicksPerSecond != 0 ? m_Scene->mAnimations[0]->mTicksPerSecond : 25.0f) * m_TimeMultiplier;
            m_AnimationTime += ts * ticksPerSecond;
            m_AnimationTime = fmod(m_AnimationTime, (float)m_Scene->mAnimations[0]->mDuration);
        }

        UpdateBones(m_AnimationTime);
    }
}

void Mesh::TraverseNodes(aiNode* node, const mat4& parentTransform)
{
    mat4 transform = parentTransform * AssimpMat4ToMat4(node->mTransformation);
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        Submesh& submesh = m_Submeshes[node->mMeshes[i]];
        submesh.Transform = transform;
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++)
        TraverseNodes(node->mChildren[i], transform);
}

void Mesh::UpdateBones(float time)
{
    TraverseNodeHierarchy(m_AnimationTime, m_Scene->mRootNode);

    for (uint32_t m = 0; m < m_Scene->mNumMeshes; m++)
    {
        const aiMesh* mesh = m_Scene->mMeshes[m];
        Submesh& submesh = m_Submeshes[m];
        auto& aabb = submesh.BoundingBox;
        aabb = AABB();

        for (uint32_t i = 0; i < mesh->mNumBones; i++)
        {
            const aiBone* bone = mesh->mBones[i];
            std::string boneName(bone->mName.data);
            if (m_BoneMap.find(boneName) != m_BoneMap.end())
            {
                uint32_t boneIndex = m_BoneMap[boneName];
                const auto& boneInfo = m_BoneInfo[boneIndex];

                vec3 min = vec3(boneInfo.FinalTransform * vec4(boneInfo.BoundingBox.Min, 1.0f));
                vec3 max = vec3(boneInfo.FinalTransform * vec4(boneInfo.BoundingBox.Max, 1.0f));

                aabb.Min.x = math::min(min.x, aabb.Min.x);
                aabb.Min.y = math::min(min.y, aabb.Min.y);
                aabb.Min.z = math::min(min.z, aabb.Min.z);
                aabb.Max.x = math::max(max.x, aabb.Max.x);
                aabb.Max.y = math::max(max.y, aabb.Max.y);
                aabb.Max.z = math::max(max.z, aabb.Max.z);
            }
        }
    }
}

void Mesh::TraverseNodeHierarchy(float time, const aiNode* node, const mat4& parentTransform)
{
    std::string nodeName(node->mName.data);
    const aiAnimation* animation = m_Scene->mAnimations[0];
    const aiNodeAnim* nodeAnim = FindNodeAnim(animation, nodeName);

    mat4 nodeTransform = AssimpMat4ToMat4(node->mTransformation);
    if (nodeAnim)
    {
        vec3 translation = InterpolateTranslation(time, nodeAnim);
        mat4 translationMatrix = mat4::translation(translation);

        quat rotation = InterpolateRotation(time, nodeAnim);
        aiQuaternion ai_rot(rotation.x, rotation.y, rotation.z, rotation.w);
        mat4 rotationMatrix = mat4::rotation(rotation);
        //rotationMatrix = AssimpMat4ToMat4(aiMatrix4x4(ai_rot.GetMatrix()));

        vec3 scale = InterpolateScale(time, nodeAnim);
        mat4 scalingMatrix = mat4::scale(scale);

        nodeTransform = translationMatrix * rotationMatrix * scalingMatrix;
    }

    mat4 transform = parentTransform * nodeTransform;

    if (m_BoneMap.find(nodeName) != m_BoneMap.end())
    {
        uint32_t boneIndex = m_BoneMap[nodeName];
        m_BoneInfo[boneIndex].FinalTransform = m_InverseRootTransform * transform * m_BoneInfo[boneIndex].Offset;
        m_BoneTransforms[boneIndex] = m_BoneInfo[boneIndex].FinalTransform;
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++)
        TraverseNodeHierarchy(time, node->mChildren[i], transform);
}

const aiNodeAnim* Mesh::FindNodeAnim(const aiAnimation* animation, const std::string& nodeName)
{
    for (uint32_t i = 0; i < animation->mNumChannels; i++)
    {
        const aiNodeAnim* nodeAnim = animation->mChannels[i];
        if (nodeName == nodeAnim->mNodeName.data)
            return nodeAnim;
    }

    return nullptr;
}

uint32_t Mesh::FindPosition(float time, const aiNodeAnim* nodeAnim)
{
    for (uint32_t i = 0; i < nodeAnim->mNumPositionKeys - 1; i++)
    {
        if (time < nodeAnim->mPositionKeys[i + 1].mTime)
            return i;
    }

    return 0;
}

uint32_t Mesh::FindRotation(float time, const aiNodeAnim* nodeAnim)
{
    for (uint32_t i = 0; i < nodeAnim->mNumRotationKeys - 1; i++)
    {
        if (time < nodeAnim->mRotationKeys[i + 1].mTime)
            return i;
    }

    return 0;
}

uint32_t Mesh::FindScaling(float time, const aiNodeAnim* nodeAnim)
{
    for (uint32_t i = 0; i < nodeAnim->mNumScalingKeys - 1; i++)
    {
        if (time < nodeAnim->mScalingKeys[i + 1].mTime)
            return i;
    }

    return 0;
}

vec3 Mesh::InterpolateTranslation(float time, const aiNodeAnim* nodeAnim)
{
    if (nodeAnim->mNumPositionKeys == 1)
    {
        auto value = nodeAnim->mPositionKeys[0].mValue;
        return {value.x, value.y, value.z};
    }

    uint32_t positionIndex = FindPosition(time, nodeAnim);
    uint32_t nextPositionIndex = positionIndex + 1;
    float factor = (time - (float)nodeAnim->mPositionKeys[positionIndex].mTime) / (float)(nodeAnim->mPositionKeys[nextPositionIndex].mTime - nodeAnim->mPositionKeys[positionIndex].mTime);
    if (factor < 0.0f)
        factor = 0.0f;

    const aiVector3D& startPosition = nodeAnim->mPositionKeys[positionIndex].mValue;
    const aiVector3D& endPosition = nodeAnim->mPositionKeys[nextPositionIndex].mValue;
    aiVector3D position = startPosition + (endPosition - startPosition) * factor;

    return {position.x, position.y, position.z};
}

quat Mesh::InterpolateRotation(float time, const aiNodeAnim* nodeAnim)
{
    if (nodeAnim->mNumRotationKeys == 1)
    {
        auto value = nodeAnim->mRotationKeys[0].mValue;
        return {value.w, value.x, value.y, value.z};
    }

    uint32_t rotationIndex = FindRotation(time, nodeAnim);
    uint32_t nextRotationIndex = rotationIndex + 1;
    float factor = (time - (float)nodeAnim->mRotationKeys[rotationIndex].mTime) / (float)(nodeAnim->mRotationKeys[nextRotationIndex].mTime - nodeAnim->mRotationKeys[rotationIndex].mTime);
    if (factor < 0.0f)
        factor = 0.0f;

    const aiQuaternion& startRotation = nodeAnim->mRotationKeys[rotationIndex].mValue;
    const aiQuaternion& endRotation = nodeAnim->mRotationKeys[nextRotationIndex].mValue;
    aiQuaternion rotation;
    aiQuaternion::Interpolate(rotation, startRotation, endRotation, factor);
    rotation.Normalize();

    return {rotation.x, rotation.y, rotation.z, rotation.w};
}

vec3 Mesh::InterpolateScale(float time, const aiNodeAnim* nodeAnim)
{
    if (nodeAnim->mNumScalingKeys == 1)
    {
        auto value = nodeAnim->mScalingKeys[0].mValue;
        return {value.x, value.y, value.z};
    }

    uint32_t scalingIndex = FindScaling(time, nodeAnim);
    uint32_t nextScalingIndex = scalingIndex + 1;
    float factor = (time - (float)nodeAnim->mScalingKeys[scalingIndex].mTime) / (float)(nodeAnim->mScalingKeys[nextScalingIndex].mTime - nodeAnim->mScalingKeys[scalingIndex].mTime);
    if (factor < 0.0f)
        factor = 0.0f;

    const aiVector3D& startScale = nodeAnim->mScalingKeys[scalingIndex].mValue;
    const aiVector3D& endScale = nodeAnim->mScalingKeys[nextScalingIndex].mValue;
    aiVector3D scale = startScale + (endScale - startScale) * factor;

    return {scale.x, scale.y, scale.z};
}