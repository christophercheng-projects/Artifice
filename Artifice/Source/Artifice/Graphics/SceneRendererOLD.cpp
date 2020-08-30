#include "SceneRenderer.h"

#include "Shader.h"
#include "Artifice/Scene/Scene.h"

#if 0

struct PushConstantEnvironmentPrefilter
{
    float Roughness;
};

struct PushConstantSkybox
{
    mat4 InverseViewProjection;
    float TextureLOD;
};

struct PushConstantComposite
{
    float Exposure;
};

static const uint32 UniformBufferSize = 256;

struct UniformBufferVertexGlobal
{
    union {
        struct
        {
            mat4 ViewProjection;
            mat4 View;
            vec3 CameraPos;
        };
        uint8 padding[UniformBufferSize];
    };
};

struct UniformBufferFragmentGlobal
{
    union {
        struct
        {
            vec4 Positions[4];
            vec4 Colors[4];
        };
        uint8 padding[UniformBufferSize];
    };
};

struct UniformBufferVertexPerModel
{
    union {
        struct
        {
            mat4 Transform;
            mat4 NormalTransform;
        };
        uint8 padding[UniformBufferSize];
    };
};

struct SceneRendererData
{
    Device* Device;
    ShaderLibrary* ShaderLibrary;

    CommandBuffer* CommandBuffer;

    RenderHandle GlobalVertexUniformBuffer;
    RenderHandle GlobalFragmentUniformBuffer;
    UniformBufferVertexGlobal* GlobalVertexPtr;
    UniformBufferFragmentGlobal* GlobalFragmentPtr;

    Material* CompositeMaterial;

    float Exposure = 1.0f;


    PBRMaterial* MaterialBase;

    const Scene* ActiveScene = nullptr;
    struct SceneInfo
    {
        PerspectiveCamera SceneCamera;
        RenderPassLayout RenderPassLayout;

        Material* SkyboxMaterial;

        PrecomputedIBLData SceneEnvironment;



    } SceneData;

    struct DrawCommand
    {
        Mesh* Mesh;
        mat4 Transform;
    };
    std::vector<DrawCommand> DrawList;

    SceneRendererOptions Options;
};

static SceneRendererData s_Data;

void SceneRenderer::Init(Device* device)
{
    s_Data.ShaderLibrary = new ShaderLibrary(device);
    s_Data.Device = device;

    LoadShaders();

    MaterialPipelineState state;
    state.VertexLayout = {};
    state.DepthTestEnabled = false;
    state.DepthWriteEnabled = false;
    s_Data.CompositeMaterial = new Material(s_Data.ShaderLibrary->Get("Composite"), state);

    // PBR
    BufferInfo ubo;
    ubo.Access = MemoryAccessType::Cpu;
    ubo.BindFlags = RenderBindFlags::UniformBuffer;
    ubo.Size = AR_FRAME_COUNT * UniformBufferSize;

    s_Data.GlobalVertexUniformBuffer = s_Data.Device->CreateBuffer(ubo);
    s_Data.GlobalFragmentUniformBuffer = s_Data.Device->CreateBuffer(ubo);

    s_Data.GlobalVertexPtr = (UniformBufferVertexGlobal*)s_Data.Device->MapBuffer(s_Data.GlobalVertexUniformBuffer);
    s_Data.GlobalFragmentPtr = (UniformBufferFragmentGlobal*)s_Data.Device->MapBuffer(s_Data.GlobalFragmentUniformBuffer);
    for (uint32 i = 0; i < AR_FRAME_COUNT; i++)
    {
        UniformBufferFragmentGlobal* ptr = s_Data.GlobalFragmentPtr + i;
        ptr->Positions[0] = vec4(-2.0f, 2.0f, 2.0f, 0.0f);
        ptr->Positions[1] = vec4(2.0f, 2.0f, 2.0f, 0.0f);
        ptr->Positions[2] = vec4(-2.0f, -2.0f, 2.0f, 0.0f);
        ptr->Positions[3] = vec4(2.0f, -2.0f, 2.0f, 0.0f);
        ptr->Colors[0] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
        ptr->Colors[1] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
        ptr->Colors[2] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
        ptr->Colors[3] = vec4(300.0f, 300.0f, 300.0f, 0.0f);
    }

    s_Data.MaterialBase = new PBRMaterial(Renderer::GetShaderLibrary()->Get("PBR_Static"));

}

void SceneRenderer::Shutdown()
{
    s_Data.MaterialBase->CleanUp();
    delete s_Data.MaterialBase;
    s_Data.CompositeMaterial->CleanUp();
    delete s_Data.CompositeMaterial;
    s_Data.ShaderLibrary->CleanUp();
    delete s_Data.ShaderLibrary;
    s_Data.Device->DestroyBuffer(s_Data.GlobalVertexUniformBuffer);
    s_Data.Device->DestroyBuffer(s_Data.GlobalFragmentUniformBuffer);


    s_Data = SceneRendererData();
}

ShaderLibrary* SceneRenderer::GetShaderLibrary()
{
    return s_Data.ShaderLibrary;
}

void SceneRenderer::SetExposure(float exposure)
{
    s_Data.Exposure = exposure;
}

void SceneRenderer::BeginScene(PrecomputedIBLData ibl, Material* skybox, PerspectiveCamera camera, RenderPassLayout rpl, CommandBuffer* cmd)
{
    s_Data.CommandBuffer = cmd;
    s_Data.SceneData.RenderPassLayout = rpl;
    s_Data.SceneData.SceneCamera = camera;
    s_Data.SceneData.SkyboxMaterial = skybox;
    s_Data.SceneData.SceneEnvironment = ibl;

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


    // Setup global descriptor set (set = 0)
    {
        DescriptorSetUpdate ds;
        ds.AddUniformBufferDynamic(0, s_Data.GlobalVertexUniformBuffer, 0, UniformBufferSize);
        ds.AddUniformBufferDynamic(1, s_Data.GlobalFragmentUniformBuffer, 0, UniformBufferSize);
        ds.AddCombinedImageSampler(2, s_Data.SceneData.SceneEnvironment.IrradianceTexture, ResourceState::SampledFragment(), s_Data.SceneData.SceneEnvironment.IrradianceSampler);
        ds.AddCombinedImageSampler(3, s_Data.SceneData.SceneEnvironment.PrefilterEnvironmentTexture, ResourceState::SampledFragment(), s_Data.SceneData.SceneEnvironment.PrefilterEnvironmentSampler);
        ds.AddCombinedImageSampler(4, s_Data.SceneData.SceneEnvironment.BRDFTexture, ResourceState::SampledFragment(), s_Data.SceneData.SceneEnvironment.BRDFSampler);

        s_Data.MaterialBase->UpdateDescriptorSet(0, ds);
    }

    UniformBufferVertexGlobal* global_vertex_ptr = s_Data.GlobalVertexPtr + s_Data.Device->GetFrameIndex();
    global_vertex_ptr->ViewProjection = camera.GetViewProjectionMatrix();
    global_vertex_ptr->View = camera.GetViewMatrix();
    global_vertex_ptr->CameraPos = camera.GetPosition();

    UniformBufferFragmentGlobal* global_fragment_ptr = s_Data.GlobalFragmentPtr + s_Data.Device->GetFrameIndex();
    for (unsigned int i = 0; i < 4; ++i)
    {
        global_fragment_ptr->Positions[i] = positions[i] + vec4(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0, 1.0f);
        global_fragment_ptr->Colors[i] = colors[i];
    }
}
void SceneRenderer::EndScene()
{
    DrawSkybox();

    s_Data.CommandBuffer->BindGraphicsPipeline(s_Data.MaterialBase->RequestPassPipeline(s_Data.SceneData.RenderPassLayout));

    uint32 offset = UniformBufferSize * s_Data.Device->GetFrameIndex();
    s_Data.CommandBuffer->BindGraphicsDescriptorSet(0, s_Data.MaterialBase->GetDescriptorSet(0), {offset, offset});
    uint32 submesh_count = 0;
    for (auto& command : s_Data.DrawList)
    {
        submesh_count += command.Mesh->GetSubmeshes().size();
    }

    ScratchBuffer ubo = s_Data.Device->RequestUniformScratchBuffer(UniformBufferSize * submesh_count);
    UniformBufferVertexPerModel* ubo_ptr = (UniformBufferVertexPerModel*)ubo.Pointer;
    offset = 0;
    for (auto& command : s_Data.DrawList)
    {
        const auto& submeshes = command.Mesh->GetSubmeshes();

        for (auto& submesh : submeshes)
        {
            mat4 transform = command.Transform * submesh.Transform;
            ubo_ptr[offset].Transform = transform;
            ubo_ptr[offset].NormalTransform = transform.inverse().transpose();
            
            offset++;
        }
    }
    DescriptorSetUpdate ds;
    ds.AddUniformBufferDynamic(0, ubo.Buffer, ubo.Offset, UniformBufferSize);
    RenderHandle per_model_ds = Renderer::GetShaderLibrary()->Get("PBR_Static")->AllocateUpdatedDescriptorSet(2, ds);
    uint32 index = 0;
    for (auto& command : s_Data.DrawList)
    {
        command.Mesh->Bind(s_Data.CommandBuffer);
        const auto& submeshes = command.Mesh->GetSubmeshes();

        auto mats = command.Mesh->GetMaterials();

        for (auto submesh : submeshes)
        {
            auto mat = mats[submesh.MaterialIndex];
            PBRMaterialBuilder build = mat->GetBuild();
            PushConstantPBR_Static pc;
            pc.Albedo = build.Albedo;
            pc.Roughness = build.Roughness;
            pc.Metallic = build.Metalness;
            pc.AO = 1.0f;
            pc.AlbedoTexToggle = build.AlbedoTexToggle;
            pc.NormalTexToggle = build.NormalTexToggle;
            pc.MetalnessTexToggle = build.MetalnessTexToggle;
            pc.RoughnessTexToggle = build.RoughnessTexToggle;

            s_Data.CommandBuffer->BindGraphicsDescriptorSet(1, mat->GetDescriptorSet(1));
            s_Data.CommandBuffer->BindGraphicsDescriptorSet(2, per_model_ds, {index * UniformBufferSize});
            s_Data.CommandBuffer->PushConstants(ShaderStage::VertexFragment, sizeof(pc), 0, &pc);

            s_Data.CommandBuffer->DrawIndexed(submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);

            index++;
        }
    }
    s_Data.Device->DestroyDescriptorSet(per_model_ds);

    s_Data.DrawList.clear();
}

void SceneRenderer::DrawComposite(RenderHandle texture, RenderPassLayout rpl, CommandBuffer* cmd)
{
    cmd->BindGraphicsPipeline(s_Data.CompositeMaterial->RequestPassPipeline(rpl));
    DescriptorSetUpdate ds;
    ds.AddCombinedImageSampler(0, texture, ResourceState::SampledFragment(), Renderer::GetWhiteSampler());
    s_Data.CompositeMaterial->UpdateDescriptorSet(0, ds);
    cmd->BindGraphicsDescriptorSet(0, s_Data.CompositeMaterial->GetDescriptorSet(0));

    PushConstantComposite pc;
    pc.Exposure = s_Data.Exposure;
    cmd->PushConstants(ShaderStage::Fragment, sizeof(pc), 0, &pc);

    cmd->Draw(3, 1, 0, 0);
}

void SceneRenderer::DrawSkybox(float skybox_lod)
{
    s_Data.CommandBuffer->BindGraphicsPipeline(s_Data.SceneData.SkyboxMaterial->RequestPassPipeline(s_Data.SceneData.RenderPassLayout));
    s_Data.CommandBuffer->BindGraphicsDescriptorSet(0, s_Data.SceneData.SkyboxMaterial->GetDescriptorSet(0));
    {
        PushConstantSkybox pc;
        pc.InverseViewProjection = (s_Data.SceneData.SceneCamera.GetProjectionMatrix() * s_Data.SceneData.SceneCamera.GetViewRotationMatrix()).inverse();
        pc.TextureLOD = skybox_lod;
        s_Data.CommandBuffer->PushConstants(ShaderStage::VertexFragment, sizeof(pc), 0, &pc);
    }
    s_Data.CommandBuffer->Draw(3, 1, 0, 0);
}

void SceneRenderer::SubmitMesh(Mesh* mesh, const mat4& transform)
{
    s_Data.DrawList.push_back({mesh, transform});
}

void SceneRenderer::LoadShaders()
{
    // IBL COMPUTE SHADERS

    // EquirectangularToCubemap
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 0;
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {1, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };
        s_Data.ShaderLibrary->Load("EquirectangularToCubemap", "Assets/Shaders/EquirectangularToCubemap.comp.spv", layout);
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
        s_Data.ShaderLibrary->Load("EnvironmentIrradiance", "Assets/Shaders/EnvironmentIrradiance.comp.spv", layout);
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
        s_Data.ShaderLibrary->Load("EnvironmentPrefilter", "Assets/Shaders/EnvironmentPrefilter.comp.spv", layout);
    }
    // BRDF LUT
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 0;
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };
        s_Data.ShaderLibrary->Load("BRDF", "Assets/Shaders/BRDF.comp.spv", layout);
    }

    // SKYBOX
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {ShaderStage::VertexFragment, sizeof(PushConstantSkybox), 0};
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment},
        };
        s_Data.ShaderLibrary->Load("Skybox", "Assets/Shaders/Skybox.vert.spv", "Assets/Shaders/Skybox.frag.spv", layout);
    }
    // Composite
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {ShaderStage::Fragment, sizeof(PushConstantSkybox), 0};
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment},
        };
        s_Data.ShaderLibrary->Load("Composite", "Assets/Shaders/Composite.vert.spv", "Assets/Shaders/Composite.frag.spv", layout);
    }
}

void SceneRenderer::DestroyEnvironmentMap(PrecomputedIBLData ibl)
{
    s_Data.Device->DestroyTexture(ibl.IrradianceTexture);
    s_Data.Device->DestroySampler(ibl.IrradianceSampler);

    s_Data.Device->DestroyTexture(ibl.PrefilterEnvironmentTexture);
    s_Data.Device->DestroySampler(ibl.PrefilterEnvironmentSampler);

    s_Data.Device->DestroyTexture(ibl.BRDFTexture);
    s_Data.Device->DestroySampler(ibl.BRDFSampler);
}

PrecomputedIBLData SceneRenderer::CreateEnvironmentMap(const std::string& filepath)
{
    PrecomputedIBLData result;

    const uint32 cubemap_size = 2048;
    const uint32 irradiance_size = 32;
    const uint32 brdf_size = 512;

    ImageData equi_data = ReadImageHDR(filepath);

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

    RenderHandle equi_sampler = s_Data.Device->CreateSampler(equi_samp);
    RenderHandle cube_sampler = s_Data.Device->CreateSampler(cube_samp);

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

        result.IrradianceTexture = s_Data.Device->CreateTexture(irradiance);
        result.IrradianceSampler = s_Data.Device->CreateSampler(irradiance_samp);
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

        result.PrefilterEnvironmentTexture = s_Data.Device->CreateTexture(prefilter);
        result.PrefilterEnvironmentSampler = s_Data.Device->CreateSampler(prefilter_samp);
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

        result.BRDFTexture = s_Data.Device->CreateTexture(brdf);
        result.BRDFSampler = s_Data.Device->CreateSampler(brdf_samp);
    }

    RenderGraph graph(s_Data.Device);

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
            RenderHandle pipeline = s_Data.ShaderLibrary->Get("EquirectangularToCubemap")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.Add(0, DescriptorType::CombinedImageSampler, 1, 0, {{registry->GetTexture("Equirectangular"), ResourceState::SampledCompute(), equi_sampler}});
                ds.Add(1, DescriptorType::StorageImage, 1, 0, {{registry->GetTexture("Cubemap"), ResourceState::StorageTextureCompute(), RenderHandle()}});
                ds0 = s_Data.ShaderLibrary->Get("EquirectangularToCubemap")->AllocateUpdatedDescriptorSet(0, ds);
            }

            cmd->BindComputePipeline(pipeline);
            cmd->BindComputeDescriptorSet(0, ds0);
            cmd->Dispatch(cubemap_size / 16, cubemap_size / 16, 6);

            s_Data.Device->DestroyComputePipeline(pipeline);
            s_Data.Device->DestroyDescriptorSet(ds0);
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
            RenderHandle pipeline = s_Data.ShaderLibrary->Get("EnvironmentIrradiance")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.Add(0, DescriptorType::CombinedImageSampler, 1, 0, {{registry->GetTexture("Cubemap"), ResourceState::SampledCompute(), cube_sampler}});
                ds.Add(1, DescriptorType::StorageImage, 1, 0, {{registry->GetTexture("Irradiance"), ResourceState::StorageTextureCompute(), RenderHandle()}});
                ds0 = s_Data.ShaderLibrary->Get("EnvironmentIrradiance")->AllocateUpdatedDescriptorSet(0, ds);
            }

            cmd->BindComputePipeline(pipeline);
            cmd->BindComputeDescriptorSet(0, ds0);
            cmd->Dispatch(irradiance_size / 16, irradiance_size / 16, 6);

            s_Data.Device->DestroyDescriptorSet(ds0);
            s_Data.Device->DestroyComputePipeline(pipeline);
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
            RenderHandle pipeline = s_Data.ShaderLibrary->Get("EnvironmentPrefilter")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.Add(0, DescriptorType::CombinedImageSampler, 1, 0, {{registry->GetTexture("Cubemap"), ResourceState::SampledCompute(), cube_sampler}});
                ds0 = s_Data.ShaderLibrary->Get("EnvironmentPrefilter")->AllocateUpdatedDescriptorSet(0, ds);
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
                RenderHandle ds1 = s_Data.ShaderLibrary->Get("EnvironmentPrefilter")->AllocateUpdatedDescriptorSet(1, ds);

                float roughness = level * deltaRoughness;

                cmd->BindComputeDescriptorSet(1, ds1);
                PushConstantEnvironmentPrefilter pc;
                pc.Roughness = roughness;
                cmd->PushConstants(ShaderStage::Compute, sizeof(pc), 0, &pc);
                cmd->Dispatch(numGroups, numGroups, 6);

                s_Data.Device->DestroyDescriptorSet(ds1);
            }

            s_Data.Device->DestroyDescriptorSet(ds0);
            s_Data.Device->DestroyComputePipeline(pipeline);
        };
    });
    graph.AddPass("BRDF Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("BRDF", result.BRDFTexture, ResourceState::None());
        builder->WriteTexture("BRDF", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            RenderHandle pipeline = s_Data.ShaderLibrary->Get("BRDF")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.Add(0, DescriptorType::StorageImage, 1, 0, {{registry->GetTexture("BRDF"), ResourceState::StorageTextureCompute(), RenderHandle()}});
                ds0 = s_Data.ShaderLibrary->Get("BRDF")->AllocateUpdatedDescriptorSet(0, ds);
            }

            cmd->BindComputePipeline(pipeline);
            cmd->BindComputeDescriptorSet(0, ds0);
            cmd->Dispatch(brdf_size / 16, brdf_size / 16, 1);

            s_Data.Device->DestroyDescriptorSet(ds0);
            s_Data.Device->DestroyComputePipeline(pipeline);
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

    s_Data.Device->DestroySampler(equi_sampler);
    s_Data.Device->DestroySampler(cube_sampler);

    Timer timer;
    s_Data.Device->WaitForFences();

    AR_CORE_INFO("Precomputed IBL data GPU time: %f ms", timer.ElapsedMillis());

    return result;
}

#endif