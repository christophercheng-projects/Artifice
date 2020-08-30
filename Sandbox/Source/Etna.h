#pragma once

#include <Artifice.h>

struct EtnaPushConstants
{
    float Time;
};

class Etna : public Layer
{
private:
    ShaderLibrary m_ShaderLibrary;
    ShaderInstance m_ShaderInstance;
    ShaderSystem m_ShaderSystem;

public:
    Etna();
    virtual ~Etna() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    void OnUpdate(Timestep ts) override;
    virtual void OnConstructRenderGraph(RenderGraph* graph) override;
    virtual void OnImGuiRender() override;
    void OnEvent(Event& e) override;

private:
};