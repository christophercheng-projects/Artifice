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

static const uint32 UniformBufferSize = 256;

struct UniformBufferVertexGlobal
{
    union {
        struct
        {
            mat4 ViewProjection;
            mat4 View;
            vec3 CameraPos;
        };
        uint8 padding[UniformBufferSize];
    };
};

struct UniformBufferFragmentGlobal
{
    union {
        struct
        {
            vec4 Positions[4];
            vec4 Colors[4];
        };
        uint8 padding[UniformBufferSize];
    };
};

struct UniformBufferVertexPerModel
{
    union {
        struct
        {
            mat4 Transform;
            mat4 NormalTransform;
        };
        uint8 padding[UniformBufferSize];
    };
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


class SandboxPBR : public Layer
{
private:
    FPSCameraController m_FPSCameraController;

    ShaderLibrary m_ShaderLibrary;
    Mesh* mesh;

    PrecomputedIBLData m_IBLData;

    Material m_SkyboxMaterialBase;
    MaterialInstance* m_SkyboxMaterial;

    std::vector<float> m_SphereVertexData;
    std::vector<uint32> m_SphereIndexData;

    RenderHandle m_GlobalVertexUniformBuffer;
    RenderHandle m_GlobalFragmentUniformBuffer;
    UniformBufferVertexGlobal* m_GlobalVertexPtr;
    UniformBufferFragmentGlobal* m_GlobalFragmentPtr;

    RenderHandle m_GlobalDescriptorSet;
    

    PBRMaterial m_PBRMaterialBase;
    MaterialInstance m_PBRMaterial;

public:
    SandboxPBR();
    virtual ~SandboxPBR() = default;

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