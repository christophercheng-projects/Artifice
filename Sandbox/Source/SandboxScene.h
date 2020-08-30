#pragma once

#include <Artifice.h>


class SandboxScene : public Layer
{
private:
    FPSCameraController m_FPSCameraController;

    EntityHandle e1;
    EntityHandle e2;
    Mesh* mesh1;
    Mesh* mesh2;

    float m_Exposure = 1.0f;
    float m_SkyboxLOD = 0.0f;


    Scene m_Scene;
public:
    SandboxScene();
    virtual ~SandboxScene() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    void OnUpdate(Timestep ts) override;
    virtual void OnConstructRenderGraph(RenderGraph* graph) override;
    virtual void OnImGuiRender() override;
    void OnEvent(Event& e) override;

private:
};