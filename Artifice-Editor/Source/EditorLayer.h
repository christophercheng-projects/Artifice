#pragma once

#include <Artifice.h>

class EditorLayer : public Layer
{
private:
    OrthographicCameraController m_OrthographicCameraController;

    ImGuiImage m_ImGuiSceneImage;
    vec2 m_ViewportSize;

    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;

public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    void OnUpdate(Timestep ts) override;
    virtual void OnConstructRenderGraph(RenderGraph* graph) override;
    virtual void OnImGuiRender() override;
    void OnEvent(Event& e) override;
};