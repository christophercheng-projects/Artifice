#pragma once

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Timestep.h"

#include "Artifice/Events/Event.h"

#include "Artifice/Graphics/RenderGraph/RenderGraph.h"
#include "Artifice/Graphics/RenderHandle.h"


class Layer
{
public:
    Layer();
    ~Layer();

    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate(Timestep ts) {}
    virtual void OnConstructRenderGraph(RenderGraph* graph) {}
    virtual std::vector<RenderHandle> GetActiveSwapchains() { return {}; }
    virtual void OnImGuiRender() {}
    virtual void OnEvent(Event& e) {}
};
