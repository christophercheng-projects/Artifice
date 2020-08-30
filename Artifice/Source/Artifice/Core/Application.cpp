#include "Application.h"

#include <iostream>


#include "Artifice/Core/Log.h"
#include "Artifice/Core/Window.h"

#include "Artifice/Debug/Instrumentor.h"

#include "Artifice/Utils/Timer.h"


#include "Artifice/Graphics/Renderer.h"
#include "Artifice/Graphics/Renderer2D.h"
#include "Artifice/Graphics/CameraController.h"


Application* Application::s_Instance = nullptr;



Application::Application()
{
    AR_PROFILE_FUNCTION();

    AR_CORE_ASSERT(!s_Instance, "Application already created");

    s_Instance = this;

    m_ScopeAllocator = new ScopeAllocator();

    m_LayerStack = new LayerStack();
    

    Window::PlatformInit();

    m_Window = new Window();
    m_Window->Init("Artifice", 1600, 900);
    m_Window->SetEventCallback([this](Event& e) -> void { this->OnEvent(e); });


    m_RenderBackend = new RenderBackend();
    m_RenderBackend->Init();

    DeviceID device_id = m_RenderBackend->CreateDevice();
    device = m_RenderBackend->GetDevice(device_id);
    device->SetVSync(false);

    m_RenderGraph = new RenderGraph(device);

    Renderer::Init(device);

    SwapchainInfo swap;
    swap.Window = m_Window->GetHandle();
    swap.Width = m_Window->GetWidth();
    swap.Height = m_Window->GetHeight();

    m_Swapchain = device->CreateSwapchain(swap);
    

    m_ImGuiLayer = new ImGuiLayer();
    PushOverlay(m_ImGuiLayer);
}

Application::~Application()
{
    device->WaitIdle();

    m_RenderGraph->CleanUp();


    device->DestroySwapchain(m_Swapchain);

    m_Window->CleanUp();
    delete m_Window;
    
    delete m_LayerStack;

    Renderer::Shutdown();
    m_RenderBackend->CleanUp();
    delete m_RenderBackend;
    

    Window::PlatformCleanUp();

    delete m_ScopeAllocator;
}

void Application::PushLayer(Layer* layer)
{
    AR_PROFILE_FUNCTION();

    m_LayerStack->PushLayer(layer);
    layer->OnAttach();
}

void Application::PushOverlay(Layer* layer)
{
    AR_PROFILE_FUNCTION();

    m_LayerStack->PushOverlay(layer);
    layer->OnAttach();
}

void Application::OnEvent(Event& e)
{
    AR_PROFILE_FUNCTION();
    
    if (m_LayerStack == nullptr) return;

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) -> bool { return this->OnWindowClose(e); });
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) -> bool { return this->OnWindowResize(e); });


    for (auto it = m_LayerStack->rbegin(); it != m_LayerStack->rend(); ++it)
    {
        (*it)->OnEvent(e);
        if (e.Handled)
        {
            break;
        }
    }
}

void Application::Run()
{
    AR_PROFILE_FUNCTION();

    unsigned int frames = 0;
    Timer performance_timer;
    Timer frame_timer;

    double last_frame_time = 0;


    while (m_Running)
    {
        AR_PROFILE_SCOPE("Runloop");

        double time = glfwGetTime();
        Timestep ts = (float)(time - last_frame_time);
        last_frame_time = time;

        //Timestep ts(frame_timer.ElapsedF());
        //frame_timer.Reset();
        
        {
            Timer timer;
            Window::PollEvents();
            m_Statistics.PollEventsTime = timer.ElapsedMillis();
        }

        {
            Timer timer;

            for (Layer* layer : *m_LayerStack)
            {
                layer->OnUpdate(ts);
            }
            m_Statistics.UpdateTime = timer.ElapsedMillis();
            timer.Reset();
        
            m_ImGuiLayer->Begin();
            for (Layer* layer : *m_LayerStack)
            {
                layer->OnImGuiRender();
            }
            m_Statistics.ImGuiUpdateTime = timer.ElapsedMillis();
            timer.Reset();
            ImGui::Render();
            m_Statistics.ImGuiRenderTime = timer.ElapsedMillis();
            timer.Reset();
            ImGui::UpdatePlatformWindows();
            m_Statistics.ImGuiUpdateWindowsTime = timer.ElapsedMillis();
        }
        // Block until GPU finished previous frame.
        m_RenderBackend->BeginFrame();


        std::vector<RenderHandle> viewports = m_ImGuiLayer->GetActiveSwapchains();
        viewports.push_back(m_Swapchain);

        bool result = m_RenderBackend->AcquireSwapchains(viewports, BIT(0));
        if (!result)
        {
            // resize occured, any resources previously referencing swapchain needs to be recreated

        }


        // stage where do we first need to access swapchain image
        for (auto vp : viewports)
        {
            device->AddWaitForAcquireSwapchainSemaphore(QueueType::Universal, vp, PipelineStageFlags::Color);
        }


        m_RenderGraph->Reset();
        m_RenderGraph->AddPass("Clear Primary Swapchain", [&](RenderGraphBuilder* builder) {
            
            builder->SetType(RenderGraphPassType::Render);
            builder->SetQueue(QueueType::Universal);

            builder->ImportTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, device->GetSwapchainTexture(m_Swapchain), {RenderBindFlags::None, PipelineStageFlags::Color});
            builder->WriteTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Color(), true);

            return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
                
            };
        });

        for (Layer* layer : *m_LayerStack)
        {
            layer->OnConstructRenderGraph(m_RenderGraph);
        }

        m_ImGuiLayer->AugmentRenderGraph(m_RenderGraph);

        m_RenderGraph->AddPass("Primary Swapchain Transition for Present", [&](RenderGraphBuilder* builder) {
            
            builder->SetType(RenderGraphPassType::Other);
            builder->SetQueue(PRESENT_QUEUE);

            builder->ReadTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Present());

            return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
                
                for (auto vp : viewports)
                {
                    device->AddSignalReleaseSwapchainSemaphore(PRESENT_QUEUE, vp);
                }
            };
        });
        m_RenderGraph->Compile();
        m_RenderGraph->Evaluate(); // Automatically submits to queues


        result = m_RenderBackend->PresentSwapchains(PRESENT_QUEUE, viewports, BIT(0));
        if (!result)
        {
            // resize occured, any resources previously referencing swapchain needs to be recreated

        }

        m_RenderBackend->EndFrame();


        frames++;

        if (performance_timer.Elapsed() > 1.0)
        {
            performance_timer.Reset();

            AR_CORE_INFO("%d FPS", frames);
            frames = 0;
        }
    }

    AR_CORE_INFO("Shutting down!");
}

void Application::Shutdown()
{
    m_Running = false;
}

bool Application::OnWindowClose(WindowCloseEvent& e)
{
    m_Running = false;
    return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e)
{
    if (e.GetWidth() == 0 || e.GetHeight() == 0)
    {
        m_Minimized = true;
        return false;
    }

    m_Minimized = false;
    
    return false;
}
