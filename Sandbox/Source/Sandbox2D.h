#pragma once

#include <Artifice.h>

class Sandbox2D : public Layer
{
private:
    OrthographicCameraController m_OrthographicCameraController;

    RenderHandle m_CheckerboardTexture;

public:
    Sandbox2D();
    virtual ~Sandbox2D() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    void OnUpdate(Timestep ts) override;
    virtual void OnConstructRenderGraph(RenderGraph* graph) override;
    virtual void OnImGuiRender() override;
    void OnEvent(Event& e) override;
};