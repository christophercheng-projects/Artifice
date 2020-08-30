#pragma once

#include <Artifice.h>

#if 0


struct PushConstantEnvironmentPrefilter
{
    float Roughness;
};

struct PushConstantSkybox
{
    mat4 InverseViewProjection;
    float TextureLOD;
};

struct UniformBufferVertexPBR
{
    mat4 ViewProjection;
    vec3 CameraPos;
};

struct UniformBufferFragmentPBR
{
    vec4 Positions[4];
    vec4 Colors[4];
};

struct PrecomputedIBLData
{
    RenderHandle PrefilterEnvironmentTexture;
    RenderHandle PrefilterEnvironmentSampler;
    RenderHandle IrradianceTexture;
    RenderHandle IrradianceSampler;
    RenderHandle BRDFTexture;
    RenderHandle BRDFSampler;
};

static const uint32 UniformBufferSize = 256;

class Sandbox3D : public Layer
{
private:
    FPSCameraController m_FPSCameraController;

    ShaderLibrary m_ShaderLibrary;
    Mesh* mesh;

    PrecomputedIBLData m_IBLData;

    Material m_SkyboxMaterialBase;
    MaterialInstance m_SkyboxMaterial;

    std::vector<float> m_SphereVertexData;
    std::vector<uint32> m_SphereIndexData;

    UniformBufferVertexPBR* m_PBRVertexUBOPtr;
    UniformBufferFragmentPBR* m_PBRFragmentUBOPtr;

    RenderHandle m_PBRVertexUBO;
    RenderHandle m_PBRFragmentUBO;

    Material m_PBRMaterialBase;
    MaterialInstance m_PBRMaterial;

public:
    Sandbox3D();
    virtual ~Sandbox3D() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    void OnUpdate(Timestep ts) override;
    virtual void OnConstructRenderGraph(RenderGraph* graph) override;
    virtual void OnImGuiRender() override;
    void OnEvent(Event& e) override;
private:
    void GenerateSpheres();
    void LoadShaders();
    PrecomputedIBLData PrecomputeIBL(const std::string& hdr_path);
};

#endif