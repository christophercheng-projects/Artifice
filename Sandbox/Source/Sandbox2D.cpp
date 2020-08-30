#include "Sandbox2D.h"

#include <stb_image.h>
#include <imgui.h>

Sandbox2D::Sandbox2D()
    : m_OrthographicCameraController((float)Application::Get()->GetWindow()->GetWidth() / Application::Get()->GetWindow()->GetHeight(), true)
{
}

void Sandbox2D::OnAttach()
{
    AR_PROFILE_FUNCTION();

    ImageData img = ReadImage("Assets/Textures/Checkerboard.png");

    TextureInfo info;
    info.Type = TextureType::Texture2D;
    info.BindFlags = RenderBindFlags::SampledTexture | RenderBindFlags::TransferSource | RenderBindFlags::TransferDestination;
    info.Width = img.Width;
    info.Height = img.Height;
    info.Format = VK_FORMAT_R8G8B8A8_UNORM;
    info.MipsEnabled = true;

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    
    m_CheckerboardTexture = device->CreateTexture(info);

    CommandBuffer cmd = device->RequestCommandBuffer(QueueType::Universal);
    cmd.Begin();
    cmd.UploadTextureData(m_CheckerboardTexture, img.Data.size(), img.Data.data(), true, ResourceState::SampledFragment());
    cmd.End();
    device->CommitCommandBuffer(cmd);
    device->SubmitQueue(QueueType::Universal, {}, {}, true);

}

void Sandbox2D::OnDetach()
{
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    device->DestroyTexture(m_CheckerboardTexture);
}

void Sandbox2D::OnConstructRenderGraph(RenderGraph* graph)
{
    AR_PROFILE_FUNCTION();

    graph->AddPass("Renderer2D Scene", [&](RenderGraphBuilder* builder) {
        
        builder->ReadWriteTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Color());
        
        TextureInfo color = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);

        TextureInfo depth;
        depth.Type = TextureType::Texture2D;
        depth.BindFlags = RenderBindFlags::DepthStencilAttachment;
        depth.Width = color.Width;
        depth.Height = color.Height;
        depth.Format = Application::Get()->GetRenderBackend()->GetDevice()->GetDefaultDepthFormat();

        builder->CreateTexture("Renderer2D Scene Depth", depth);
        builder->WriteTexture("Renderer2D Scene Depth", ResourceState::Depth(), true);
        
        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            
            cmd->SetViewport(color.Width, color.Height);
            cmd->SetScissor(0, 0, color.Width, color.Height);

            Renderer2D::BeginScene(m_OrthographicCameraController.GetCamera().GetViewProjectionMatrix(), rpl, cmd);
            
            //Renderer2D::DrawQuad({0.0f, 0.0f, 0.5f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
            Renderer2D::DrawQuad({0.0f, 0.0f, 0.5f}, {20.0f, 20.0f}, m_CheckerboardTexture, 10.0f);
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

void Sandbox2D::OnUpdate(Timestep ts)
{
    AR_PROFILE_FUNCTION();

    m_OrthographicCameraController.OnUpdate(ts);
}

void Sandbox2D::OnImGuiRender()
{
    
    ImGui::Begin("Settings");

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
}

void Sandbox2D::OnEvent(Event& e)
{
    m_OrthographicCameraController.OnEvent(e);
}