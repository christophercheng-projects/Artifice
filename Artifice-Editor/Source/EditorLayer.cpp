#include "EditorLayer.h"

#include <imgui.h>

EditorLayer::EditorLayer()
    : m_OrthographicCameraController((float)Application::Get()->GetWindow()->GetWidth() / Application::Get()->GetWindow()->GetHeight(), true)
{
}

void EditorLayer::OnAttach()
{
    AR_PROFILE_FUNCTION();

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f); // Window background
    colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.5f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.3f, 0.3f, 0.3f, 0.5f); // Widget backgrounds
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4f, 0.4f, 0.4f, 0.4f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 0.6f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.4f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.7f, 0.7f, 0.7f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.7f, 0.7f, 0.7f, 0.8f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.5f, 0.52f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.5f, 0.5f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.43f, 0.35f, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.6f, 0.15f, 1.0f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
    colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.6f, 0.6f, 1.0f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);

    m_ImGuiSceneImage = ImGuiImage("Scene Color");
}

void EditorLayer::OnDetach()
{
    
}

void EditorLayer::OnConstructRenderGraph(RenderGraph* graph)
{
    AR_PROFILE_FUNCTION();
    if (!(m_ViewportSize.x > 0 && m_ViewportSize.y > 0))
    {
        return;
    }

    graph->AddPass("Renderer2D Scene", [&](RenderGraphBuilder* builder) {
        
        
        uint32 width = m_ViewportSize.x;
        uint32 height = m_ViewportSize.y;
        
        TextureInfo color;
        color.Type = TextureType::Texture2D;
        color.Width = width;
        color.Height = height;
        color.Format = VK_FORMAT_R8G8B8A8_UNORM;

        TextureInfo depth;
        depth.Type = TextureType::Texture2D;
        depth.Width = width;
        depth.Height = height;
        depth.Format = Application::Get()->GetRenderBackend()->GetDevice()->GetDefaultDepthFormat();

        builder->CreateTexture(m_ImGuiSceneImage.RenderGraphName, color);
        builder->CreateTexture("Renderer2D Scene Depth", depth);

        builder->WriteTexture(m_ImGuiSceneImage.RenderGraphName, ResourceState::Color(), true);
        builder->WriteTexture("Renderer2D Scene Depth", ResourceState::Depth(), true);

        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            
            cmd->SetViewport(width, height);
            cmd->SetScissor(0, 0, width, height);
            
            Renderer2D::BeginScene(m_OrthographicCameraController.GetCamera().GetViewProjectionMatrix(), rpl, cmd);
            for (float y = -5.0f; y < 5.0f; y += 0.5f)
            {
                for (float x = -5.0f; x < 5.0f; x += 0.5f)
                {
                    vec4 color = {(x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f};
                    Renderer2D::DrawQuad({x, y, 0.0f}, {0.45f, 0.45f}, color);
                }
            }
            Renderer2D::EndScene();
        };
    });
}

void EditorLayer::OnUpdate(Timestep ts)
{
    AR_PROFILE_FUNCTION();

    if (m_ViewportFocused)
        m_OrthographicCameraController.OnUpdate(ts);

}

void EditorLayer::OnImGuiRender()
{
    AR_PROFILE_FUNCTION();
    {
        static bool dockspaceOpen = true;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

                if (ImGui::MenuItem("Exit"))
                    Application::Get()->Shutdown();

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::Begin("Settings");

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        {
            Application::Statistics stats = Application::Get()->GetStats();
            ImGui::Text("Application Stats:");
            ImGui::Text("Poll events time: %f", stats.PollEventsTime);
            ImGui::Text("Update time: %f", stats.UpdateTime);
            ImGui::Text("ImGui update time: %f", stats.ImGuiUpdateTime);
            ImGui::Text("ImGui render time: %f", stats.ImGuiRenderTime);
            ImGui::Text("ImGui update windows time: %f", stats.ImGuiUpdateWindowsTime);
        }
        {
            Renderer2D::Statistics stats = Renderer2D::GetStats();
            ImGui::Text("Renderer2D Stats:");
            ImGui::Text("Draw Calls: %d", stats.DrawCalls);
            ImGui::Text("Quads: %d", stats.QuadCount);
            ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
            ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
            Renderer2D::ResetStats();
        }
        {
            RenderGraph::Statistics stats = Application::Get()->GetRenderGraph()->GetStats();
            ImGui::Text("Render Graph Stats:");
            ImGui::Text("Total Passes: %d", stats.TotalPassCount);
            ImGui::Text("Render Passes: %d", stats.RenderPassCount);
            ImGui::Text("Textures: %d", stats.TextureCount);
            ImGui::Text("Buffers: %d", stats.BufferCount);
            ImGui::Text("Alive Framebuffers: %d", stats.AliveFramebufferCount);
            ImGui::Text("Alive Render Passes: %d", stats.RenderPassCount);
            ImGui::Text("Alive Textures: %d", stats.AliveTextureCount);
            ImGui::Text("Alive Buffers: %d", stats.AliveTextureCount);
            ImGui::Text("Construction time: %f", stats.ConstructionTime);
            ImGui::Text("Evaluation time: %f", stats.EvaluationTime);
            for (uint32 i = 0; i < stats.PassNames.size(); i++)
            {
                ImGui::BulletText("'%s: %f'", stats.PassNames[i].c_str(), stats.PassTimes[i]);
            }
            Application::Get()->GetRenderGraph()->ResetStats();
        }
        

        ImGui::End();

        ImGui::Begin("Viewport");

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();

        Application::Get()->GetImGuiLayer()->BlockEvents(!m_ViewportFocused || !m_ViewportHovered);

        ImVec2 viewport_size = ImGui::GetContentRegionAvail();
        m_ViewportSize = {viewport_size.x, viewport_size.y};

        if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
        {
            ImGui::Image(&m_ImGuiSceneImage, viewport_size);
            m_OrthographicCameraController.SetAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
        }

        ImGui::End();

        ImGui::End();
    }
}

void EditorLayer::OnEvent(Event& e)
{
    m_OrthographicCameraController.OnEvent(e);
}