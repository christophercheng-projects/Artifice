#pragma once

#include <Artifice.h>
#if 0
class AtmosphereLayer : public Layer
{
private:

    RenderGraph* m_PrecomputeGraph;
    Shader m_TransmittanceShader;
    Shader m_ScatteringShader;

    Material m_TransmittanceComputeMaterial;
    Material m_ScatteringComputeMaterial;

    RenderHandle m_TransmittanceComputeSampler;

    RenderHandle m_TransmittanceTexture;
    RenderHandle m_ScatteringTexture;

public:
    AtmosphereLayer();
    virtual ~AtmosphereLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    void OnUpdate(Timestep ts) override;
    virtual void OnConstructRenderGraph(RenderGraph* graph) override;
    virtual void OnImGuiRender() override;
    void OnEvent(Event& e) override;
};
#endif