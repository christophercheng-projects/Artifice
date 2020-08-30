#pragma once

#include <Artifice.h>
#include "Atmosphere.h"


class SandboxAtmosphere : public Layer
{
private:
    Atmosphere m_Atmosphere;

    FPSCameraController m_CameraController;
    
    ShaderLibrary m_ShaderLibrary;

    ShaderInstance m_SkyShaderInstance;
    ShaderSystem m_SkyShaderSystem;
    ShaderInstance m_CubemapShaderInstance;
    ShaderSystem m_CubemapShaderSystem;

    float m_SunRotation = 0.0f;
    float m_Exposure = 10.0f;

    EntityHandle e;
    Mesh* mesh;

    Scene m_Scene;
    RenderHandle m_Cubemap;
    RenderHandle m_Skybox;
    RenderHandle m_SkyboxSampler;

public:
    SandboxAtmosphere();
    virtual ~SandboxAtmosphere() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    void OnUpdate(Timestep ts) override;
    virtual void OnConstructRenderGraph(RenderGraph* graph) override;
    virtual void OnImGuiRender() override;
    void OnEvent(Event& e) override;

private:
    void LoadShaders();
    void RenderToCubemap(RenderGraph* graph, RenderHandle texture);
};