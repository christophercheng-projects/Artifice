#pragma once

#include "Artifice/Core/Core.h"
#include "Artifice/Core/ScratchAllocator.h"
#include "Artifice/Core/Layer.h"
#include "Artifice/Core/LayerStack.h"
#include "Artifice/Core/Window.h"

#include "Artifice/Events/Event.h"
#include "Artifice/Events/ApplicationEvent.h"
#include "Artifice/Events/KeyEvent.h"
#include "Artifice/Events/MouseEvent.h"

#include "Artifice/ImGui/ImGuiLayer.h"

#include "Artifice/Graphics/Material.h"
#include "Artifice/Graphics/RenderHandle.h"

#include "Artifice/Graphics/Device.h"
#include "Artifice/Graphics/CameraController.h"
#include "Artifice/Graphics/RenderBackend.h"

#include "Artifice/Graphics/RenderGraph/RenderGraph.h"


class Device;
class RenderBackend;

class Application
{
private:
	static Application* s_Instance;
private:
    RenderGraph* m_RenderGraph;

    ScopeAllocator* m_ScopeAllocator;
    
    RenderBackend* m_RenderBackend;
    Device* device;
    Window* m_Window;
    RenderHandle m_Swapchain;

    ImGuiLayer* m_ImGuiLayer;
    LayerStack* m_LayerStack;
    bool m_Running = true;
    bool m_Minimized = false;
    
public:
    Application();
    ~Application();

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    void OnEvent(Event& e);

    void Run();
    void Shutdown();

    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

    RenderGraph* GetRenderGraph() { return m_RenderGraph; }
    RenderBackend* GetRenderBackend() { return m_RenderBackend; }
    Window* GetWindow() { return m_Window; }
    RenderHandle GetSwapchain() { return m_Swapchain; }

    ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }


    static Application* Get() { return s_Instance; }


    static ScratchAllocator* RequestScopeAllocator()
    {
        return Get()->m_ScopeAllocator->RequestScope();
    }
    static void ReturnScopeAllocator(ScratchAllocator* allocator)
    {
        Get()->m_ScopeAllocator->ReturnScope(allocator);
    }

public:
    struct Statistics
    {
        float PollEventsTime;
        float UpdateTime;
        float ImGuiUpdateTime;
        float ImGuiRenderTime;
        float ImGuiUpdateWindowsTime;
    };

private:
    Statistics m_Statistics;

public:
    Statistics GetStats() const { return m_Statistics; }
};


class ScopedAllocator
{
    ScratchAllocator* m_Allocator;

public:
    ScopedAllocator()
    {
        m_Allocator = Application::RequestScopeAllocator();
    }
    ~ScopedAllocator()
    {
        Application::ReturnScopeAllocator(m_Allocator);
    }

    byte* Allocate(uint64 size)
    {
        return m_Allocator->Allocate(size);
    }

    template <typename T>
    T* Allocate(uint64 count)
    {
        return m_Allocator->Allocate<T>(count);
    }

    template <typename T>
    void Destroy(uint64 count, T* objects)
    {
        m_Allocator->Destroy<T>(count, objects);
    }
};

template <class T>
class ScopedArray
{
    ScopedAllocator m_Allocator;
    T* m_Data;
    uint32 m_Count = 0;

public:
    ScopedArray(uint32 count, ScopedAllocator allocator) : m_Allocator(allocator), m_Count(count)
    {
        m_Data = m_Allocator.Allocate<T>(m_Count);
    }
    ~ScopedArray()
    {
        m_Allocator.Destroy<T>(m_Count, m_Data);
    }
    T* GetData()
    {
        return m_Data;
    }
    void GetCount()
    {
        return m_Count;
    }

    T& operator[](uint32 index)
    {
        return m_Data[index];
    }

    T operator[](uint32 index) const
    {
        return m_Data[index];
    }
};