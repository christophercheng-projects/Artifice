#include "Etna.h"

#include <imgui.h>
#include <stb_image.h>

Etna::Etna()
{
}

void Etna::OnAttach()
{
    AR_PROFILE_FUNCTION();

    m_ShaderLibrary = ShaderLibrary(Application::Get()->GetRenderBackend()->GetDevice());

    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {ShaderStage::Compute, sizeof(EtnaPushConstants), 0};
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };

        m_ShaderLibrary.Load("Etna", "Assets/Shaders/Etna.comp.spv", layout);
    }
    {
        ShaderSystemLayout layout;
        layout.RegisterStorageTexture(0);
        layout.RegisterPushConstants<EtnaPushConstants>(ShaderStage::Compute);

        m_ShaderSystem = ShaderSystem(layout, m_ShaderLibrary.Get("Etna"), 0);
    }

    m_ShaderInstance = ShaderInstance(m_ShaderLibrary.Get("Etna"));

}

void Etna::OnDetach()
{
    AR_PROFILE_FUNCTION();

    m_ShaderInstance.CleanUp();
    m_ShaderSystem.Shutdown();
    m_ShaderLibrary.CleanUp();

}

void Etna::OnConstructRenderGraph(RenderGraph* graph)
{
    AR_PROFILE_FUNCTION();

    const uint32 workgroup_size = 32;

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    graph->AddPass("Etna", [&](RenderGraphBuilder* builder) {

        TextureInfo swap = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);

        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        TextureInfo color;
        color.Type = TextureType::Texture2D;
        color.Width = swap.Width;
        color.Height = swap.Height;
        color.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
        builder->CreateTexture("Color", color);

        builder->WriteTexture("Color", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            
            m_ShaderInstance.BindComputePipeline(cmd);
            
            m_ShaderSystem.SetStorageTexture(0, registry->GetTexture("Color"));
            m_ShaderSystem.SetPushConstants<EtnaPushConstants>({(float)glfwGetTime()});
            m_ShaderSystem.BindCompute(cmd);

            cmd->Dispatch(swap.Width / workgroup_size, swap.Height / workgroup_size, 1);
        };
    });
    graph->AddPass("PBR Composite", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Render);

        builder->ReadTexture("Color", ResourceState::SampledFragment());
        builder->ReadWriteTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Color());

        TextureInfo color = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);
        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->SetViewport(color.Width, color.Height);
            cmd->SetScissor(0, 0, color.Width, color.Height);

            SceneRenderer::SetExposure(1.0f);
            SceneRenderer::DrawComposite(registry->GetTexture("Color"), rpl, cmd);
        };
    });
}

void Etna::OnUpdate(Timestep ts)
{
    AR_PROFILE_FUNCTION();

}

void Etna::OnImGuiRender()
{
    AR_PROFILE_FUNCTION();

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

void Etna::OnEvent(Event& e)
{

}