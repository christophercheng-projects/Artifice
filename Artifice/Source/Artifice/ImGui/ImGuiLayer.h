#pragma once

#include <imgui.h>

#include "Artifice/Core/Layer.h"

#include "Artifice/Events/ApplicationEvent.h"
#include "Artifice/Events/KeyEvent.h"
#include "Artifice/Events/MouseEvent.h"

#include "Artifice/Graphics/RenderHandle.h"
#include "Artifice/Graphics/Material.h"

#include "Artifice/math/math.h"


struct ImGuiImage
{
    enum class Type { RenderGraph, Real } Type; 
    RenderHandle Texture;
    std::string RenderGraphName;

    ImGuiImage() = default;

    ImGuiImage(RenderHandle texture) : Type(Type::Real), Texture(texture) {}
    ImGuiImage(std::string render_graph_name) : Type(Type::RenderGraph), RenderGraphName(render_graph_name) {}
};


class ImGuiLayer : public Layer
{
public:
    ImGuiLayer();
    ~ImGuiLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnEvent(Event& e) override;
    

    void Begin();
    void End(RenderPassLayout rpl, RenderGraphRegistry* registry, CommandBuffer* cmd);

    void AugmentRenderGraph(RenderGraph* graph);

    void RenderImGuiDrawData(ImDrawData* draw_data, RenderPassLayout rpl, RenderGraphRegistry* registry, CommandBuffer* cmd);

    void BlockEvents(bool block) { m_BlockEvents = block; }

    virtual std::vector<RenderHandle> GetActiveSwapchains() override;

private:
    bool m_BlockEvents = true;

    ImGuiImage m_ImGuiFontImage;
    RenderHandle m_FontTexture;
    RenderHandle m_Sampler;
    RenderPassLayout m_RenderPassLayout;
    Shader* m_Shader;
    ShaderInstance m_ShaderInstance;
    ShaderSystem m_TextureSystem;
};
