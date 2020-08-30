#include "SandboxScene.h"

#include <imgui.h>
#include <stb_image.h>

SandboxScene::SandboxScene()
    : m_FPSCameraController(16.0f / 9.0f, 60.0f, 0.01f, 1000.0f)
{
}

void SandboxScene::OnAttach()
{
    AR_PROFILE_FUNCTION();

    m_Scene.Init();
    m_Scene.SetEnvironment(SceneRenderer::CreateEnvironmentMap("Assets/Textures/birchwood_4k.hdr"));

    mesh1 = new Mesh("Assets/cerberus/CerberusFix.fbx");
    mesh2 = new Mesh("Assets/TestScene.fbx");

    e1 = m_Scene.GetRegistry()->CreateEntity();
    e2 = m_Scene.GetRegistry()->CreateEntity();

    m_Scene.GetRegistry()->AddComponent<MeshComponent>(e1, {mesh1});
    m_Scene.GetRegistry()->AddComponent<MeshComponent>(e2, {mesh2});
    m_Scene.GetRegistry()->AddComponent<TransformComponent>(e1, {mat4::translation({-5.0f, 5.0f, 0.0f}) * mat4::scale(0.1f)});
}

void SandboxScene::OnDetach()
{
    AR_PROFILE_FUNCTION();

    m_Scene.GetRegistry()->DestroyEntity(e1);
    m_Scene.GetRegistry()->DestroyEntity(e2);
    
    delete mesh1; 
    delete mesh2; 
    m_Scene.CleanUp();
}

void SandboxScene::OnConstructRenderGraph(RenderGraph* graph)
{
    AR_PROFILE_FUNCTION();

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    graph->AddPass("PBR Scene", [&](RenderGraphBuilder* builder) {

        TextureInfo swap = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);
        
        TextureInfo color;
        color.Type = TextureType::Texture2D;
        color.Width = swap.Width;
        color.Height = swap.Height;
        color.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
        color.Samples = device->GetMaxSamples();
        
        TextureInfo resolve;
        resolve.Type = TextureType::Texture2D;
        resolve.Width = swap.Width;
        resolve.Height = swap.Height;
        resolve.Format = VK_FORMAT_R16G16B16A16_SFLOAT;

        TextureInfo depth;
        depth.Type = TextureType::Texture2D;
        depth.Width = swap.Width;
        depth.Height = swap.Height;
        depth.Format = device->GetDefaultDepthFormat();
        depth.Samples = device->GetMaxSamples();

        builder->CreateTexture("PBR Scene MS Color", color);
        builder->CreateTexture("PBR Scene MS Depth", depth);
        builder->CreateTexture("PBR Scene Color", resolve);

        builder->WriteTexture("PBR Scene MS Color", ResourceState::Color(), true);
        builder->WriteTexture("PBR Scene MS Depth", ResourceState::Depth(), true);
        builder->WriteTexture("PBR Scene Color", ResourceState::Resolve());

        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->SetViewport(swap.Width, swap.Height);
            cmd->SetScissor(0, 0, swap.Width, swap.Height);

            // SceneRenderer::BeginScene(m_Scene.GetEnvironment(), m_Scene.GetSkyboxMaterial(), m_FPSCameraController.GetCamera(), rpl, cmd);
            
            // SceneRenderer::SubmitMesh(mesh2, mat4::scale({0.1f, 0.1f, 0.1f}));
            // SceneRenderer::SubmitMesh(mesh1, mat4::translation({2.0f, 0.0f, 0.0f}));

            // SceneRenderer::EndScene();

            SceneRenderer::SetSkyboxLOD(m_SkyboxLOD);

            m_Scene.Render(cmd, m_FPSCameraController.GetCamera(), rpl);
            #if 0
            Renderer2D::BeginScene(m_FPSCameraController.GetCamera().GetViewProjectionMatrix(), rpl, cmd);
            Renderer2D::DrawLine({0, 0, 0}, {100, 100, 100}, {1, 0, 0, 1});
            for (float y = -5.0f; y < 5.0f; y += 0.5f)
            {
                for (float x = -5.0f; x < 5.0f; x += 0.5f)
                {
                    vec4 color = {(x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f};
                    Renderer2D::DrawQuad({x, y, 0.0f}, {0.45f, 0.45f}, color);
                }
            }
            Renderer2D::EndScene();
            #endif

        };
    });
    graph->AddPass("PBR Composite", [&](RenderGraphBuilder* builder) {
        builder->ReadTexture("PBR Scene Color", ResourceState::SampledFragment());
        builder->ReadWriteTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Color());

        TextureInfo color = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);
        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->SetViewport(color.Width, color.Height);
            cmd->SetScissor(0, 0, color.Width, color.Height);

            SceneRenderer::SetExposure(m_Exposure);
            SceneRenderer::DrawComposite(registry->GetTexture("PBR Scene Color"), rpl, cmd);
        };
    });
}

void SandboxScene::OnUpdate(Timestep ts)
{
    AR_PROFILE_FUNCTION();

    m_FPSCameraController.OnUpdate(ts);
    m_Scene.GetRegistry()->Advance();

    mesh1->OnUpdate(ts);
    mesh2->OnUpdate(ts);
}

void SandboxScene::OnImGuiRender()
{
    AR_PROFILE_FUNCTION();

    ImGui::Begin("SceneRenderer Settings");
    ImGui::SliderFloat("SkyboxLOD", &m_SkyboxLOD, 0.0f, 5.0f);
    ImGui::SliderFloat("Exposure", &m_Exposure, 0.5f, 1.0f);
    ImGui::End();
    
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

void SandboxScene::OnEvent(Event& e)
{
    m_FPSCameraController.OnEvent(e);
}