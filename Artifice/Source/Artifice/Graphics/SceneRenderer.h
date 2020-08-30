#pragma once

#include "Artifice/math/math.h"

#include "Artifice/Graphics/Camera.h"

#include "Artifice/Graphics/RenderGraph/RenderGraph.h"
#include "Artifice/Graphics/RenderHandle.h"
#include "Artifice/Graphics/Mesh.h"
#include "Artifice/Graphics/Resources.h"


struct PrecomputedIBLData
{
    RenderHandle PrefilterEnvironmentTexture;
    RenderHandle PrefilterEnvironmentSampler;
    RenderHandle IrradianceTexture;
    RenderHandle IrradianceSampler;
};

struct PrecomputedBRDFData
{
    RenderHandle Texture;
    RenderHandle Sampler;
};

struct SceneRendererOptions
{
    bool ShowGrid = true;
    float GridResolution = 0.025f;
    float GridScale = 16.025f;
    float GridSize = 16.025f;
};

class SceneRenderer
{
public:
    static void Init(Device* device);
    static void Shutdown();

    static void BeginScene(PrecomputedIBLData ibl, PerspectiveCamera camera, RenderPassLayout rpl, CommandBuffer* cmd);
    static void EndScene();

    static void DrawComposite(RenderHandle texture, RenderPassLayout rpl, CommandBuffer* cmd);

    static void SubmitMesh(Mesh* mesh, const mat4& transform = mat4::identity());

    static PrecomputedIBLData CreateEnvironmentMap(const std::string& filepath);
    static PrecomputedIBLData CreateEnvironmentMap(RenderGraph* graph, const std::string& cube_name, uint32 cube_size);
    static void DestroyEnvironmentMap(PrecomputedIBLData ibl);

    static PrecomputedBRDFData CreateBRDF(uint32 size = 512);
    static void DestroyBRDF(PrecomputedBRDFData brdf);

    static void SetSkybox(RenderHandle skybox_texture, RenderHandle skybox_Sampler);

    static void SetExposure(float exposure);
    static void SetSkyboxLOD(float lod);


    static SceneRendererOptions& GetOptions();

    static ShaderLibrary* GetShaderLibrary();

private:
    static void LoadShaders();
};