#include "SandboxAtmosphere.h"

#include <imgui.h>
#include <stb_image.h>


#define WORKGROUP_SIZE_2D 8
#define WORKGROUP_SIZE_3D 4

#define DYNAMIC_CUBEMAP_SIZE 32

struct SkyPushConstants
{
    mat4 InverseViewProjection;
    vec3 CameraPosition;
    float Padding;
    vec3 SunDirection;
    float Exposure;
};

SandboxAtmosphere::SandboxAtmosphere()
    : m_CameraController(16.0f / 9.0f, 60.0f, 0.01f, 10000.0f)
{


}

void SandboxAtmosphere::OnAttach()
{
    AR_PROFILE_FUNCTION();

    m_Atmosphere.Init();

    LoadShaders();

    // Cubemap texture
    TextureInfo cube;
    cube.Type = TextureType::TextureCube;
    cube.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::TransferDestination | RenderBindFlags::TransferSource | RenderBindFlags::SampledTexture;
    cube.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
    cube.Width = DYNAMIC_CUBEMAP_SIZE;
    cube.Height = DYNAMIC_CUBEMAP_SIZE;
    cube.Layers = 6;
    cube.MipsEnabled = true;
    m_Cubemap = Application::Get()->GetRenderBackend()->GetDevice()->CreateTexture(cube);
    
    cube.Width = 1024;
    cube.Height = 1024;
    m_Skybox = Application::Get()->GetRenderBackend()->GetDevice()->CreateTexture(cube);

    SamplerInfo samp;
    samp.Filter = VK_FILTER_LINEAR;
    samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samp.Mips = MIPS(cube.Width, cube.Height);
    m_SkyboxSampler = Application::Get()->GetRenderBackend()->GetDevice()->CreateSampler(samp);

    // Set uniforms
    
    AtmosphereTextures textures = m_Atmosphere.GetTextures();

    m_SkyShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Atmosphere.GetParameters());
    m_SkyShaderSystem.SetCombinedTextureSampler(1, textures.TransmittanceTexture, textures.Sampler);
    m_SkyShaderSystem.SetCombinedTextureSampler(2, textures.IrradianceTexture, textures.Sampler);
    m_SkyShaderSystem.SetCombinedTextureSampler(3, textures.ScatteringTexture, textures.Sampler);

    m_CubemapShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Atmosphere.GetParameters());
    m_CubemapShaderSystem.SetCombinedTextureSampler(1, textures.TransmittanceTexture, textures.Sampler);
    m_CubemapShaderSystem.SetCombinedTextureSampler(2, textures.IrradianceTexture, textures.Sampler);
    m_CubemapShaderSystem.SetCombinedTextureSampler(3, textures.ScatteringTexture, textures.Sampler);
    m_CubemapShaderSystem.SetCombinedTextureSampler(3, textures.ScatteringTexture, textures.Sampler);
    m_CubemapShaderSystem.SetStorageTexture(4, m_Cubemap);

    // Pipeline
    m_SkyShaderInstance = ShaderInstance(m_ShaderLibrary.Get("Sky"), {});
    m_CubemapShaderInstance = ShaderInstance(m_ShaderLibrary.Get("Cubemap"));


    // Setup scene
    m_Scene.Init();
    //m_Scene.SetEnvironment(SceneRenderer::CreateEnvironmentMap("Assets/Textures/birchwood_4k.hdr"));

    mesh = new Mesh("Assets/TestScene.fbx");

    e = m_Scene.GetRegistry()->CreateEntity();

    m_Scene.GetRegistry()->AddComponent<MeshComponent>(e, {mesh});
}

void SandboxAtmosphere::LoadShaders()
{
    m_ShaderLibrary = ShaderLibrary(Application::Get()->GetRenderBackend()->GetDevice());

    // Render sky
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {
            ShaderStage::Fragment, sizeof(SkyPushConstants), 0
        };
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Fragment},
            {1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment},
            {2, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment},
            {3, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment},
        };

        m_ShaderLibrary.Load("Sky", "Assets/Shaders/Atmosphere/Sky.vert.spv", "Assets/Shaders/Atmosphere/Sky.frag.spv", layout);
    }
    {
        ShaderSystemLayout layout;
        layout.RegisterUniformBuffer<AtmosphereParameters>(0);
        layout.RegisterCombinedTextureSampler(1);
        layout.RegisterCombinedTextureSampler(2);
        layout.RegisterCombinedTextureSampler(3);
        layout.RegisterPushConstants<SkyPushConstants>(ShaderStage::Fragment);

        m_SkyShaderSystem = ShaderSystem(layout, m_ShaderLibrary.Get("Sky"), 0);
    }
    // Cubemap
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {
            ShaderStage::Compute, sizeof(SkyPushConstants), 0
        };
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Compute},
            {1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {2, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {3, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {4, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };

        m_ShaderLibrary.Load("Cubemap", "Assets/Shaders/Atmosphere/Cubemap.comp.spv", layout);
    }
    {
        ShaderSystemLayout layout;
        layout.RegisterUniformBuffer<AtmosphereParameters>(0);
        layout.RegisterCombinedTextureSampler(1);
        layout.RegisterCombinedTextureSampler(2);
        layout.RegisterCombinedTextureSampler(3);
        layout.RegisterStorageTexture(4);
        layout.RegisterPushConstants<SkyPushConstants>(ShaderStage::Compute);

        m_CubemapShaderSystem = ShaderSystem(layout, m_ShaderLibrary.Get("Cubemap"), 0);
    }
}

void SandboxAtmosphere::OnDetach()
{
    AR_PROFILE_FUNCTION();

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    device->DestroyTexture(m_Cubemap);
    device->DestroyTexture(m_Skybox);
    device->DestroySampler(m_SkyboxSampler);

    m_Atmosphere.Shutdown();
    m_Scene.CleanUp();

    m_SkyShaderInstance.CleanUp();
    m_SkyShaderSystem.Shutdown();

    m_CubemapShaderInstance.CleanUp();
    m_CubemapShaderSystem.Shutdown();
    
    m_ShaderLibrary.CleanUp();
}

void SandboxAtmosphere::OnConstructRenderGraph(RenderGraph* graph)
{
    AR_PROFILE_FUNCTION();
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    RenderToCubemap(graph, m_Cubemap);
    m_Scene.SetEnvironment(SceneRenderer::CreateEnvironmentMap(graph, "Cubemap", DYNAMIC_CUBEMAP_SIZE));

    
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
        //depth.Samples = device->GetMaxSamples();

        builder->CreateTexture("PBR Scene MS Color", color);
        //builder->CreateTexture("PBR Scene MS Depth", depth);
        builder->CreateTexture("PBR Scene Depth", depth);
        builder->CreateTexture("PBR Scene Color", resolve);

        //builder->WriteTexture("PBR Scene MS Color", ResourceState::Color(), true);
        //builder->WriteTexture("PBR Scene MS Depth", ResourceState::Depth(), true);
        //builder->WriteTexture("PBR Scene Color", ResourceState::Resolve());
        builder->WriteTexture("PBR Scene Color", ResourceState::Color());
        builder->WriteTexture("PBR Scene Depth", ResourceState::Depth(), true);

        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->SetViewport(swap.Width, swap.Height);
            cmd->SetScissor(0, 0, swap.Width, swap.Height);

            m_Scene.SetSkybox(m_Skybox, m_SkyboxSampler);
            SceneRenderer::SetSkyboxLOD(0.0f);

            m_Scene.Render(cmd, m_CameraController.GetCamera(), rpl);
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
#if 0
    graph->AddPass("Atmosphere", [&](RenderGraphBuilder* builder) {
        builder->WriteTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Color(), true);

        TextureInfo color = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);
        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->SetViewport(color.Width, color.Height);
            cmd->SetScissor(0, 0, color.Width, color.Height);

            SkyPushConstants pc;
            pc.InverseViewProjection = (m_CameraController.GetCamera().GetProjectionMatrix() * m_CameraController.GetCamera().GetViewRotationMatrix()).inverse();
            pc.CameraPosition = m_CameraController.GetCamera().GetPosition();
            float rads = m_SunRotation * 3.14159265f / 180.0f;
            pc.SunDirection = vec3(cos(rads), sin(rads), 0).Normalize();
            pc.Exposure = m_Exposure;

            m_SkyShaderSystem.SetPushConstants<SkyPushConstants>(pc);

            m_SkyShaderInstance.BindGraphicsPipeline(cmd, rpl);
            m_SkyShaderSystem.Bind(cmd);

            cmd->Draw(3, 1, 0, 0);
        };
    });
#endif
}

void SandboxAtmosphere::RenderToCubemap(RenderGraph* graph, RenderHandle texture)
{
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    
    #if 0
    graph->AddPass("Import Cubemap", [&](RenderGraphBuilder* builder) {
        
        builder->SetType(RenderGraphPassType::Other);
        builder->ImportTexture("Cubemap", texture);

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {

        };
    });
    #endif
    graph->AddPass("Render to Cubemap", [&](RenderGraphBuilder* builder) {
        
        builder->SetQueue(QueueType::Compute);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("Cubemap", texture);
        builder->WriteTexture("Cubemap", ResourceState::StorageTextureCompute());
        
        TextureInfo info = builder->GetTextureInfo("Cubemap");

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            
            SkyPushConstants pc;
            pc.CameraPosition = m_CameraController.GetCamera().GetPosition();
            float rads = m_SunRotation * 3.14159265f / 180.0f;
            pc.SunDirection = vec3(cos(rads), sin(rads), 0).Normalize();
            pc.Exposure = m_Exposure;

            m_CubemapShaderSystem.SetStorageTexture(4, texture);
            m_CubemapShaderSystem.SetPushConstants<SkyPushConstants>(pc);

            m_CubemapShaderInstance.BindComputePipeline(cmd);
            m_CubemapShaderSystem.BindCompute(cmd);
            cmd->Dispatch(info.Width / WORKGROUP_SIZE_2D, info.Height / WORKGROUP_SIZE_2D, 6);
        };
    });
    graph->AddPass("Render to Skybox", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Compute);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("Skybox", m_Skybox);
        builder->WriteTexture("Skybox", ResourceState::StorageTextureCompute());

        TextureInfo info = builder->GetTextureInfo("Skybox");

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            SkyPushConstants pc;
            pc.CameraPosition = m_CameraController.GetCamera().GetPosition();
            float rads = m_SunRotation * 3.14159265f / 180.0f;
            pc.SunDirection = vec3(cos(rads), sin(rads), 0).Normalize();
            pc.Exposure = m_Exposure;
            
            m_CubemapShaderSystem.SetStorageTexture(4, m_Skybox);
            m_CubemapShaderSystem.SetPushConstants<SkyPushConstants>(pc);

            m_CubemapShaderInstance.BindComputePipeline(cmd);
            m_CubemapShaderSystem.BindCompute(cmd);
            cmd->Dispatch(info.Width / WORKGROUP_SIZE_2D, info.Height / WORKGROUP_SIZE_2D, 6);
        };
    });
    graph->AddPass("Generate Skybox Mips Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Compute);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadWriteTexture("Skybox", ResourceState::TransferDestination());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->GenerateTextureMips(registry->GetTexture("Skybox"));
        };
    });

#if 0
    mat4 projection = mat4::perspective(1.0f, 90.0f, 0.01f, 1000.0f);

    for (uint32 i = 0; i < 6; i++)
    {
        mat4 view;
        switch (i)
        {
        case 0: // POSITIVE_X
            view = mat4::rotation(90.0f, vec3(0.0f, 1.0f, 0.0f)) * mat4::rotation(180.0f, vec3(1.0f, 0.0f, 0.0f));
            break;
        case 1: // NEGATIVE_X
            view = mat4::rotation(-90.0f, vec3(0.0f, 1.0f, 0.0f)) * mat4::rotation(180.0f, vec3(1.0f, 0.0f, 0.0f));
            break;
        case 2: // POSITIVE_Y
            view = mat4::rotation(-90.0f, vec3(1.0f, 0.0f, 0.0f));
            break;
        case 3: // NEGATIVE_Y
            view = mat4::rotation(90.0f, vec3(1.0f, 0.0f, 0.0f));
            break;
        case 4: // POSITIVE_Z
            view = mat4::rotation(180.0f, vec3(1.0f, 0.0f, 0.0f));
            break;
        case 5: // NEGATIVE_Z
            view = mat4::rotation(180.0f, vec3(0.0f, 0.0f, 1.0f));
            break;
        }

        std::cout << i << " " << (projection * view).inverse() << std::endl;

        graph->AddPass("Render to Cubemap Face" + std::to_string(i), [&](RenderGraphBuilder* builder) {
            
            builder->ReadWriteTexture("Cubemap", ResourceState::Color(), i);

            TextureInfo info = builder->GetTextureInfo("Cubemap");
            RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

            return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
                cmd->SetViewport(info.Width, info.Height);
                cmd->SetScissor(0, 0, info.Width, info.Height);

                SkyPushConstants pc;
                pc.InverseViewProjection = (projection * view).inverse();
                pc.CameraPosition = m_CameraController.GetCamera().GetPosition();
                float rads = m_SunRotation * 3.14159265f / 180.0f;
                pc.SunDirection = vec3(cos(rads), sin(rads), 0).Normalize();
                pc.Exposure = m_Exposure;

                m_SkyShaderSystem.SetPushConstants<SkyPushConstants>(pc);

                m_SkyShaderInstance.BindGraphicsPipeline(cmd, rpl);
                m_SkyShaderSystem.Bind(cmd);

                cmd->Draw(3, 1, 0, 0);
            };
        });
    }
    #endif

    graph->AddPass("Transition Skybox for Rendering", [&](RenderGraphBuilder* builder) {
        builder->SetType(RenderGraphPassType::Other);
        builder->ReadTexture("Skybox", ResourceState::SampledFragment());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {

        };
    });
}

void SandboxAtmosphere::OnUpdate(Timestep ts)
{
    AR_PROFILE_FUNCTION();
    if (Input::IsKeyPressed(KeyCode::Right))
    {
        m_SunRotation += 20.0f * ts;
        if (m_SunRotation > 180.0f)
        {
            m_SunRotation = -180.0f;
        }
    }
    else if (Input::IsKeyPressed(KeyCode::Left))
    {
        m_SunRotation -= 20.0f * ts;
        if (m_SunRotation < -180.0f)
        {
            m_SunRotation = 180.0f;
        }
    }

    m_CameraController.OnUpdate(ts);
}

void SandboxAtmosphere::OnImGuiRender()
{
    AR_PROFILE_FUNCTION();

    ImGui::Begin("Settings");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::SliderFloat("Exposure", &m_Exposure, 0.0f, 10.0f);
    ImGui::SliderFloat("Sun Rotation", &m_SunRotation, -180.0f, 180.0f);
    ImGui::End();

    ImGui::Begin("Settings");

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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

void SandboxAtmosphere::OnEvent(Event& e)
{
    AR_PROFILE_SCOPE();

    m_CameraController.OnEvent(e);
}
