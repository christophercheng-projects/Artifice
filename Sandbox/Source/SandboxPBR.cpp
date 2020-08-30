#if 0
#include "SandboxPBR.h"

#include <imgui.h>
#include <stb_image.h>

SandboxPBR::SandboxPBR()
    : m_FPSCameraController(16.0f / 9.0f, 60.0f, 0.01f, 1000.0f)
{

}

void SandboxPBR::OnAttach()
{
    AR_PROFILE_FUNCTION();

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    // Load shaders
    LoadShaders();

    // Generate sphere vertex/index data used for PBR rendering
    GenerateSpheres();
    //mesh = new Mesh("Assets/cerberus/CerberusFix.fbx");

    // Precompute IBL data (irradiance map, prefiltered radiance map, brdf lut)
    m_IBLData = PrecomputeIBL("Assets/Textures/pink_sunrise_4k.hdr");

    // Skybox (use prefiltered map as background)
    m_SkyboxMaterialBase = Material(m_ShaderLibrary.Get("Skybox"));
    {
        MaterialPipelineState state;
        state.VertexLayout = {};
        state.DepthTestEnabled = false;
        state.DepthWriteEnabled = false;
        m_SkyboxMaterialBase.SetMaterialPipelineState(state);
    }
    m_SkyboxMaterial = new MaterialInstance(&m_SkyboxMaterialBase, {0});
    {
        DescriptorSetUpdate ds0;
        ds0.Add(0, DescriptorType::CombinedImageSampler, 1, 0, {{m_IBLData.PrefilterEnvironmentTexture, ResourceState::SampledFragment(), m_IBLData.PrefilterEnvironmentSampler}});
        m_SkyboxMaterial->UpdateDescriptorSet(0, ds0);
    }
    
    // PBR
    BufferInfo ubo;
    ubo.Access = MemoryAccessType::Cpu;
    ubo.BindFlags = RenderBindFlags::UniformBuffer;
    ubo.Size = AR_FRAME_COUNT * UniformBufferSize;

    m_GlobalVertexUniformBuffer = device->CreateBuffer(ubo);
    m_GlobalFragmentUniformBuffer = device->CreateBuffer(ubo);

    m_GlobalVertexPtr = (UniformBufferVertexGlobal*)device->MapBuffer(m_GlobalVertexUniformBuffer);
    m_GlobalFragmentPtr = (UniformBufferFragmentGlobal*)device->MapBuffer(m_GlobalFragmentUniformBuffer);
    for (uint32 i = 0; i < AR_FRAME_COUNT; i++)
    {
        UniformBufferFragmentGlobal* ptr = m_GlobalFragmentPtr + i;
        ptr->Positions[0] = vec4(-2.0f, 2.0f, 2.0f, 0.0f);
        ptr->Positions[1] = vec4(2.0f, 2.0f, 2.0f, 0.0f);
        ptr->Positions[2] = vec4(-2.0f, -2.0f, 2.0f, 0.0f);
        ptr->Positions[3] = vec4(2.0f, -2.0f, 2.0f, 0.0f);
        ptr->Colors[0] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
        ptr->Colors[1] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
        ptr->Colors[2] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
        ptr->Colors[3] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
    }

    m_PBRMaterialBase = PBRMaterial(Renderer::GetShaderLibrary()->Get("PBR_Static"));

    // Setup global descriptor set (set = 0)
    {
        DescriptorSetUpdate ds;
        ds.AddUniformBufferDynamic(0, m_GlobalVertexUniformBuffer, 0, UniformBufferSize);
        ds.AddUniformBufferDynamic(1, m_GlobalFragmentUniformBuffer, 0, UniformBufferSize);
        ds.AddCombinedImageSampler(2, m_IBLData.IrradianceTexture, ResourceState::SampledFragment(), m_IBLData.IrradianceSampler);
        ds.AddCombinedImageSampler(3, m_IBLData.PrefilterEnvironmentTexture, ResourceState::SampledFragment(), m_IBLData.PrefilterEnvironmentSampler);
        ds.AddCombinedImageSampler(4, m_IBLData.BRDFTexture, ResourceState::SampledFragment(), m_IBLData.BRDFSampler);

        m_GlobalDescriptorSet = Renderer::GetShaderLibrary()->Get("PBR_Static")->AllocateUpdatedDescriptorSet(0, ds);
    }
    // shaders cache descriptor sets?

}

void SandboxPBR::OnDetach()
{
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    
    device->DestroyDescriptorSet(m_GlobalDescriptorSet);

    device->DestroyBuffer(m_GlobalVertexUniformBuffer);
    device->DestroyBuffer(m_GlobalFragmentUniformBuffer);


    device->DestroyTexture(m_IBLData.IrradianceTexture);
    device->DestroySampler(m_IBLData.IrradianceSampler);

    device->DestroyTexture(m_IBLData.PrefilterEnvironmentTexture);
    device->DestroySampler(m_IBLData.PrefilterEnvironmentSampler);

    device->DestroyTexture(m_IBLData.BRDFTexture);
    device->DestroySampler(m_IBLData.BRDFSampler);

    delete mesh;
    mesh = nullptr;

    m_SkyboxMaterialBase.CleanUp();

    m_PBRMaterialBase.CleanUp();

    m_ShaderLibrary.CleanUp();
}

void SandboxPBR::OnConstructRenderGraph(RenderGraph* graph)
{
    AR_PROFILE_FUNCTION();

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
#if 0
    graph->AddPass("Skybox Pass", [&](RenderGraphBuilder* builder) {
        builder->ReadWriteTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Color());

        TextureInfo color = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);

        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
           
            cmd->SetViewport(color.Width, color.Height);
            cmd->SetScissor(0, 0, color.Width, color.Height);

            // Draw skybox

            cmd->BindGraphicsPipeline(m_SkyboxMaterial->GetMaterial()->RequestPassPipeline(rpl));
            cmd->BindGraphicsDescriptorSet(0, m_SkyboxMaterial->GetDescriptorSet(0));
            {
                PushConstantSkybox pc;
                pc.InverseViewProjection = (m_FPSCameraController.GetCamera().GetProjectionMatrix() * m_FPSCameraController.GetCamera().GetViewRotationMatrix()).inverse();
                pc.TextureLOD = 0.0f;
                cmd->PushConstants(ShaderStage::VertexFragment, sizeof(pc), 0, &pc);
            }
            cmd->Draw(3, 1, 0, 0);
        };
    });
#endif
    graph->AddPass("PBR Scene", [&](RenderGraphBuilder* builder) {
        builder->ReadWriteTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Color());

        TextureInfo color = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);

        TextureInfo depth;
        depth.Type = TextureType::Texture2D;
        depth.BindFlags = RenderBindFlags::DepthStencilAttachment;
        depth.Width = color.Width;
        depth.Height = color.Height;
        depth.Format = device->GetDefaultDepthFormat();

        builder->CreateTexture("PBR Scene Depth", depth);
        builder->WriteTexture("PBR Scene Depth", ResourceState::Depth(), true);

        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
           
            cmd->SetViewport(color.Width, color.Height);
            cmd->SetScissor(0, 0, color.Width, color.Height);

            // Skybox

            cmd->BindGraphicsPipeline(m_SkyboxMaterial->GetMaterial()->RequestPassPipeline(rpl));
            cmd->BindGraphicsDescriptorSet(0, m_SkyboxMaterial->GetDescriptorSet(0));
            {
                PushConstantSkybox pc;
                pc.InverseViewProjection = (m_FPSCameraController.GetCamera().GetProjectionMatrix() * m_FPSCameraController.GetCamera().GetViewRotationMatrix()).inverse();
                pc.TextureLOD = 0.0f;
                cmd->PushConstants(ShaderStage::VertexFragment, sizeof(pc), 0, &pc);
            }
            cmd->Draw(3, 1, 0, 0);

            // Draw PBR scene
            UniformBufferVertexGlobal* global_vertex_ptr = m_GlobalVertexPtr + device->GetFrameIndex();
            global_vertex_ptr->ViewProjection = m_FPSCameraController.GetCamera().GetViewProjectionMatrix();
            global_vertex_ptr->View = m_FPSCameraController.GetCamera().GetViewMatrix();
            global_vertex_ptr->CameraPos = m_FPSCameraController.GetCamera().GetPosition();
            
            UniformBufferFragmentGlobal* global_fragment_ptr = m_GlobalFragmentPtr + device->GetFrameIndex();

            vec4 positions[4];
            vec4 colors[4];
            positions[0] = vec4(-2.0f, 2.0f, 2.0f, 0.0f);
            positions[1] = vec4(2.0f, 2.0f, 2.0f, 0.0f);
            positions[2] = vec4(-2.0f, -2.0f, 2.0f, 0.0f);
            positions[3] = vec4(2.0f, -2.0f, 2.0f, 0.0f);
            colors[0] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
            colors[1] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
            colors[2] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
            colors[3] = vec4(300.0f, 300.0f, 300.0f, 0.0f);

            // render light source (simply re-render sphere at light positions)
            // this looks a bit off as we use the same shader, but it'll make their positions obvious and
            // keeps the codeprint small.
            for (unsigned int i = 0; i < 4; ++i)
            {
                global_fragment_ptr->Positions[i] = positions[i] + vec4(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0, 1.0f);
                global_fragment_ptr->Colors[i] = colors[i];
            }


            cmd->BindGraphicsPipeline(m_PBRMaterialBase.RequestPassPipeline(rpl));

            uint32 offset = UniformBufferSize * device->GetFrameIndex();
            cmd->BindGraphicsDescriptorSet(0, m_GlobalDescriptorSet, {offset, offset});

            PushConstantPBR_Static pc;

            const auto& submeshes = mesh->GetSubmeshes();
            uint32 submesh_count = submeshes.size();

            ScratchBuffer ubo = device->RequestUniformScratchBuffer(UniformBufferSize * submesh_count);
            UniformBufferVertexPerModel* ubo_ptr = (UniformBufferVertexPerModel*)ubo.Pointer;
            for (uint32 i = 0; i < submesh_count; i++)
            {
                ubo_ptr[i].Transform = submeshes[i].Transform;
                ubo_ptr[i].NormalTransform = submeshes[i].Transform.inverse().transpose();
            }

            DescriptorSetUpdate ds;
            ds.AddUniformBufferDynamic(0, ubo.Buffer, ubo.Offset, UniformBufferSize);
            RenderHandle per_model_ds = Renderer::GetShaderLibrary()->Get("PBR_Static")->AllocateUpdatedDescriptorSet(2, ds);

            mesh->Bind(cmd);

            auto mats = mesh->GetMaterials();

            uint32 index = 0;
            for (auto submesh : submeshes)
            {
                auto mat = mats[submesh.MaterialIndex];
                PBRMaterialBuilder build = mat->GetBuild();
                pc.Albedo = build.Albedo;
                pc.Roughness = build.Roughness;
                pc.Metallic = build.Metalness;
                pc.AO = 1.0f;
                pc.AlbedoTexToggle = build.AlbedoTexToggle;
                pc.NormalTexToggle = build.NormalTexToggle;
                pc.MetalnessTexToggle = build.MetalnessTexToggle;
                pc.RoughnessTexToggle = build.RoughnessTexToggle;

                cmd->BindGraphicsDescriptorSet(1, mat->GetDescriptorSet(1));
                cmd->BindGraphicsDescriptorSet(2, per_model_ds, {index * UniformBufferSize});
                cmd->PushConstants(ShaderStage::VertexFragment, sizeof(pc), 0, &pc);

                cmd->DrawIndexed(submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);

                index++;
            }

            device->DestroyDescriptorSet(per_model_ds);
        };
    });
}

void SandboxPBR::OnUpdate(Timestep ts)
{
    AR_PROFILE_FUNCTION();

    m_FPSCameraController.OnUpdate(ts);
}

void SandboxPBR::OnImGuiRender()
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

void SandboxPBR::OnEvent(Event& e)
{
    m_FPSCameraController.OnEvent(e);
}

void SandboxPBR::LoadShaders()
{
    // Load shaders
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    m_ShaderLibrary = ShaderLibrary(device);

    // COMPUTE SHADERS

    // EquirectangularToCubemap
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 0;
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {1, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };
        m_ShaderLibrary.Load("EquirectangularToCubemap", "Assets/Shaders/EquirectangularToCubemap.comp.spv", layout);
    }
    // EnvironmentIrradiance
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 0;
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {1, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };
        m_ShaderLibrary.Load("EnvironmentIrradiance", "Assets/Shaders/EnvironmentIrradiance.comp.spv", layout);
    }
    // EnvironmentPrefilter
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {ShaderStage::Compute, sizeof(PushConstantEnvironmentPrefilter), 0};
        layout.DescriptorSetLayoutCount = 2;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
        };
        layout.DescriptorSetLayouts[1] = {
            {0, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };
        m_ShaderLibrary.Load("EnvironmentPrefilter", "Assets/Shaders/EnvironmentPrefilter.comp.spv", layout);
    }
    // BRDF LUT
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 0;
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };
        m_ShaderLibrary.Load("BRDF", "Assets/Shaders/BRDF.comp.spv", layout);
    }

    // GRAPHICS SHADERS

    // Skybox
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {ShaderStage::VertexFragment, sizeof(PushConstantSkybox), 0};
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment},
        };
        m_ShaderLibrary.Load("Skybox", "Assets/Shaders/Skybox.vert.spv", "Assets/Shaders/Skybox.frag.spv", layout);
    }
    // PBR
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {ShaderStage::VertexFragment, sizeof(PushConstantPBR), 0};
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Vertex},   // viewprojection, view, cameraposition
            {1, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Fragment}, // lights
            {2, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // irradiance map
            {3, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // prefilter map
            {4, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // brdf lut
        };
        m_ShaderLibrary.Load("PBR", "Assets/Shaders/PBR.vert.spv", "Assets/Shaders/PBR.frag.spv", layout);
    }
}

PrecomputedIBLData SandboxPBR::PrecomputeIBL(const std::string& hdr_path)
{
    PrecomputedIBLData result;

    const uint32 cubemap_size = 2048;
    const uint32 irradiance_size = 32;
    const uint32 brdf_size = 512;

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    ImageData equi_data = ReadImageHDR(hdr_path);

    SamplerInfo equi_samp;
    equi_samp.Filter = VK_FILTER_LINEAR;
    equi_samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    equi_samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    equi_samp.Mips = MIPS(equi_data.Width, equi_data.Height);

    SamplerInfo cube_samp;
    cube_samp.Filter = VK_FILTER_LINEAR;
    cube_samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    cube_samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    cube_samp.Mips = MIPS(cubemap_size, cubemap_size);

    RenderHandle equi_sampler = device->CreateSampler(equi_samp);
    RenderHandle cube_sampler = device->CreateSampler(cube_samp);

    // Create irradiance texture/sampler
    {
        TextureInfo irradiance;
        irradiance.Type = TextureType::TextureCube;
        irradiance.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination | RenderBindFlags::TransferSource;
        irradiance.Width = irradiance_size;
        irradiance.Height = irradiance_size;
        irradiance.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
        irradiance.Layers = 6;
        irradiance.MipsEnabled = true;

        SamplerInfo irradiance_samp;
        irradiance_samp.Filter = VK_FILTER_LINEAR;
        irradiance_samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        irradiance_samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        irradiance_samp.Mips = MIPS(irradiance_size, irradiance_size);

        result.IrradianceTexture = device->CreateTexture(irradiance);
        result.IrradianceSampler = device->CreateSampler(irradiance_samp);
    }
    // Create prefilter texture/sampler
    {
        TextureInfo prefilter;
        prefilter.Type = TextureType::TextureCube;
        prefilter.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination | RenderBindFlags::TransferSource;
        prefilter.Width = cubemap_size;
        prefilter.Height = cubemap_size;
        prefilter.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
        prefilter.Layers = 6;
        prefilter.MipsEnabled = true;

        SamplerInfo prefilter_samp;
        prefilter_samp.Filter = VK_FILTER_LINEAR;
        prefilter_samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        prefilter_samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        prefilter_samp.Mips = MIPS(cubemap_size, cubemap_size);

        result.PrefilterEnvironmentTexture = device->CreateTexture(prefilter);
        result.PrefilterEnvironmentSampler = device->CreateSampler(prefilter_samp);
    }
    // Create brdf texture/sampler
    {
        TextureInfo brdf;
        brdf.Type = TextureType::Texture2D;
        brdf.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination | RenderBindFlags::TransferSource;
        brdf.Width = brdf_size;
        brdf.Height = brdf_size;
        brdf.Format = VK_FORMAT_R16G16_SFLOAT;
        brdf.MipsEnabled = true;

        SamplerInfo brdf_samp;
        brdf_samp.Filter = VK_FILTER_LINEAR;
        brdf_samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        brdf_samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        brdf_samp.Mips = MIPS(brdf_size, brdf_size);

        result.BRDFTexture = device->CreateTexture(brdf);
        result.BRDFSampler = device->CreateSampler(brdf_samp);
    }

    RenderGraph graph(device);

    graph.AddPass("Upload Equirectangular Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        TextureInfo equi;
        equi.Type = TextureType::Texture2D;
        equi.BindFlags = RenderBindFlags::TransferSource;
        equi.Width = equi_data.Width;
        equi.Height = equi_data.Height;
        equi.Format = VK_FORMAT_R32G32B32A32_SFLOAT;
        equi.MipsEnabled = true;

        builder->CreateTexture("Equirectangular", equi);
        builder->WriteTexture("Equirectangular", ResourceState::TransferDestination());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->UploadTextureData(registry->GetTexture("Equirectangular"), equi_data.Data.size(), equi_data.Data.data(), true);
        };
    });
    graph.AddPass("EquirectangularToCubemap Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        TextureInfo cube;
        cube.Type = TextureType::TextureCube;
        cube.BindFlags = RenderBindFlags::TransferSource;
        cube.Width = cubemap_size;
        cube.Height = cubemap_size;
        cube.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
        cube.Layers = 6;
        cube.MipsEnabled = true;
        builder->CreateTexture("Cubemap", cube);

        builder->ReadTexture("Equirectangular", ResourceState::SampledCompute());
        builder->WriteTexture("Cubemap", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            RenderHandle pipeline = m_ShaderLibrary.Get("EquirectangularToCubemap")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.Add(0, DescriptorType::CombinedImageSampler, 1, 0, {{registry->GetTexture("Equirectangular"), ResourceState::SampledCompute(), equi_sampler}});
                ds.Add(1, DescriptorType::StorageImage, 1, 0, {{registry->GetTexture("Cubemap"), ResourceState::StorageTextureCompute(), RenderHandle()}});
                ds0 = m_ShaderLibrary.Get("EquirectangularToCubemap")->AllocateUpdatedDescriptorSet(0, ds);
            }

            cmd->BindComputePipeline(pipeline);
            cmd->BindComputeDescriptorSet(0, ds0);
            cmd->Dispatch(cubemap_size / 16, cubemap_size / 16, 6);

            device->DestroyComputePipeline(pipeline);
            device->DestroyDescriptorSet(ds0);
        };
    });
    graph.AddPass("Generate Cubemap Mips Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadWriteTexture("Cubemap", ResourceState::TransferDestination());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->GenerateTextureMips(registry->GetTexture("Cubemap"));
        };
    });
    graph.AddPass("Irradiance Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("Irradiance", result.IrradianceTexture, ResourceState::None());

        builder->ReadTexture("Cubemap", ResourceState::SampledCompute());
        builder->WriteTexture("Irradiance", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            RenderHandle pipeline = m_ShaderLibrary.Get("EnvironmentIrradiance")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.Add(0, DescriptorType::CombinedImageSampler, 1, 0, {{registry->GetTexture("Cubemap"), ResourceState::SampledCompute(), cube_sampler}});
                ds.Add(1, DescriptorType::StorageImage, 1, 0, {{registry->GetTexture("Irradiance"), ResourceState::StorageTextureCompute(), RenderHandle()}});
                ds0 = m_ShaderLibrary.Get("EnvironmentIrradiance")->AllocateUpdatedDescriptorSet(0, ds);
            }

            cmd->BindComputePipeline(pipeline);
            cmd->BindComputeDescriptorSet(0, ds0);
            cmd->Dispatch(irradiance_size / 16, irradiance_size / 16, 6);

            device->DestroyDescriptorSet(ds0);
            device->DestroyComputePipeline(pipeline);
        };
    });
    graph.AddPass("Generate Irradiance Mips Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadWriteTexture("Irradiance", ResourceState::TransferDestination());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->GenerateTextureMips(registry->GetTexture("Irradiance"));
        };
    });
    graph.AddPass("Prefilter Copy Base Mip Level Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("Prefilter", result.PrefilterEnvironmentTexture, ResourceState::None());

        builder->ReadTexture("Cubemap", ResourceState::TransferSource());
        builder->WriteTexture("Prefilter", ResourceState::TransferDestination());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->CopyTexture(registry->GetTexture("Cubemap"), registry->GetTexture("Prefilter"));
        };
    });
    graph.AddPass("Prefilter Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadTexture("Cubemap", ResourceState::SampledCompute());
        builder->WriteTexture("Prefilter", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            RenderHandle pipeline = m_ShaderLibrary.Get("EnvironmentPrefilter")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.Add(0, DescriptorType::CombinedImageSampler, 1, 0, {{registry->GetTexture("Cubemap"), ResourceState::SampledCompute(), cube_sampler}});
                ds0 = m_ShaderLibrary.Get("EnvironmentPrefilter")->AllocateUpdatedDescriptorSet(0, ds);
            }

            cmd->BindComputePipeline(pipeline);
            cmd->BindComputeDescriptorSet(0, ds0);

            const uint32_t mipCount = MIPS(cubemap_size, cubemap_size);
            const float deltaRoughness = 1.0f / math::max((float)mipCount - 1.0f, 1.0f);
            for (uint32_t level = 1, size = cubemap_size / 2; level < mipCount; level++, size /= 2)
            {
                uint32_t numGroups = math::max((uint32_t)1, size / 16);

                DescriptorSetUpdate ds;
                ds.Add(0, DescriptorType::StorageImage, 1, 0, {{registry->GetTexture("Prefilter"), ResourceState::StorageTextureCompute(), RenderHandle(), (int32)level}});
                RenderHandle ds1 = m_ShaderLibrary.Get("EnvironmentPrefilter")->AllocateUpdatedDescriptorSet(1, ds);

                float roughness = level * deltaRoughness;

                cmd->BindComputeDescriptorSet(1, ds1);
                PushConstantEnvironmentPrefilter pc;
                pc.Roughness = roughness;
                cmd->PushConstants(ShaderStage::Compute, sizeof(pc), 0, &pc);
                cmd->Dispatch(numGroups, numGroups, 6);

                device->DestroyDescriptorSet(ds1);
            }

            device->DestroyDescriptorSet(ds0);
            device->DestroyComputePipeline(pipeline);
        };
    });
    graph.AddPass("BRDF Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("BRDF", result.BRDFTexture, ResourceState::None());
        builder->WriteTexture("BRDF", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            RenderHandle pipeline = m_ShaderLibrary.Get("BRDF")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.Add(0, DescriptorType::StorageImage, 1, 0, {{registry->GetTexture("BRDF"), ResourceState::StorageTextureCompute(), RenderHandle()}});
                ds0 = m_ShaderLibrary.Get("BRDF")->AllocateUpdatedDescriptorSet(0, ds);
            }

            cmd->BindComputePipeline(pipeline);
            cmd->BindComputeDescriptorSet(0, ds0);
            cmd->Dispatch(brdf_size / 16, brdf_size / 16, 1);

            device->DestroyDescriptorSet(ds0);
            device->DestroyComputePipeline(pipeline);
        };
    });
    graph.AddPass("Generate BRDF Mips Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadWriteTexture("BRDF", ResourceState::TransferDestination());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->GenerateTextureMips(registry->GetTexture("BRDF"));
        };
    });
    graph.AddPass("Transition for Rendering", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadTexture("Irradiance", ResourceState::SampledFragment());
        builder->ReadTexture("Prefilter", ResourceState::SampledFragment());
        builder->ReadTexture("BRDF", ResourceState::SampledFragment());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {

        };
    });

    graph.Compile();
    graph.Evaluate();
    graph.CleanUp();

    device->DestroySampler(equi_sampler);
    device->DestroySampler(cube_sampler);

    Timer timer;
    device->WaitForFences();

    AR_CORE_INFO("Precomputed IBL data GPU time: %f ms", timer.ElapsedMillis());

    return result;
}

void SandboxPBR::GenerateSpheres()
{
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2> uv;
    std::vector<unsigned int> indices;

    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    const float PI = 3.14159265359;
    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
    {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            positions.push_back(vec3(xPos, yPos, zPos));
            uv.push_back(vec2(xSegment, ySegment));
            normals.push_back(vec3(xPos, yPos, zPos));
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
    {
        if (!oddRow) // even rows: y == 0, y == 2; and so on
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                indices.push_back(y * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else
        {
            for (int x = X_SEGMENTS; x >= 0; --x)
            {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }

    std::vector<float> data;
    for (unsigned int i = 0; i < positions.size(); ++i)
    {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);
        if (normals.size() > 0)
        {
            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);
        }
        if (uv.size() > 0)
        {
            data.push_back(uv[i].x);
            data.push_back(uv[i].y);
        }
    }

    m_SphereVertexData = data;
    m_SphereIndexData = indices;
};

#endif