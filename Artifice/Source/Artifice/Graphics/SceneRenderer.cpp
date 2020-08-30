#include "SceneRenderer.h"

#include "Shader.h"
#include "Artifice/Scene/Scene.h"

#include "ShaderSystem.h"

struct EnvironmentPrefilterPushConstants
{
    float Roughness;
};

struct SkyboxPushConstants
{
    mat4 InverseViewProjection;
    float TextureLOD;
};

struct CompositePushConstants
{
    float Exposure;
};

struct ViewUniformBuffer
{
    mat4 ViewProjection;
    mat4 View;
    vec3 CameraPos;
};
struct LightsUniformBuffer
{
    vec4 Positions[4];
    vec4 Colors[4];
};
struct MeshUniformBuffer
{
    mat4 BoneTransform[100];
};
struct SubmeshUniformBuffer
{
    mat4 Transform;
    mat4 NormalTransform;
};


struct SceneRendererData
{
    Device* Device;
    ShaderLibrary* ShaderLibrary;

    CommandBuffer* CommandBuffer;



    ShaderSystem StaticGlobalSystem;
    ShaderSystem StaticSubmeshSystem;

    ShaderSystem AnimatedGlobalSystem;
    ShaderSystem AnimatedMeshSystem;
    ShaderSystem AnimatedSubmeshSystem;

    float SkyboxLOD = 0.0f;
    ShaderInstance SkyboxShaderInstance;
    ShaderSystem SkyboxSystem;

    float Exposure = 1.0f;
    ShaderInstance CompositeShaderInstance;
    ShaderSystem CompositeSystem;

    ShaderInstance PBRStaticShaderInstance;
    ShaderInstance PBRAnimatedShaderInstance;

    const Scene* ActiveScene = nullptr;
    struct SceneInfo
    {
        PerspectiveCamera SceneCamera;
        RenderPassLayout RenderPassLayout;

        PrecomputedIBLData SceneEnvironment;
    } SceneData;

    struct DrawCommand
    {
        Mesh* Mesh;
        mat4 Transform;
    };
    std::vector<DrawCommand> DrawList;

    SceneRendererOptions Options;

    PrecomputedBRDFData BRDF;
};

static SceneRendererData s_Data;

void SceneRenderer::Init(Device* device)
{
    AR_PROFILE_FUNCTION();

    s_Data.ShaderLibrary = new ShaderLibrary(device);
    s_Data.Device = device;

    LoadShaders();
    {
        ShaderPipelineState state;
        state.VertexLayout = {
            {
                Format::RGB32Float, // Position
                Format::RGB32Float, // Normal
                Format::RGB32Float, // Tangent
                Format::RGB32Float, // Binormal
                Format::RG32Float,  // TexCoord
            },
        };
        state.PrimitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        state.CullMode = VK_CULL_MODE_BACK_BIT;
        state.DepthTestEnabled = true;
        state.DepthWriteEnabled = true;
        //state.Samples = device->GetMaxSamples();

        s_Data.PBRStaticShaderInstance = ShaderInstance(Renderer::GetShaderLibrary()->Get("PBR_Static"), state);
    }
    {
        ShaderPipelineState state;
        state.VertexLayout = {
            {
                Format::RGB32Float,  // Position
                Format::RGB32Float,  // Normal
                Format::RGB32Float,  // Tangent
                Format::RGB32Float,  // Binormal
                Format::RG32Float,   // TexCoord
                Format::RGBA32Int,   // BoneIndices
                Format::RGBA32Float, // BoneWeights
            },
        };
        state.PrimitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        state.CullMode = VK_CULL_MODE_BACK_BIT;
        state.DepthTestEnabled = true;
        state.DepthWriteEnabled = true;
        //state.Samples = device->GetMaxSamples();

        s_Data.PBRAnimatedShaderInstance = ShaderInstance(Renderer::GetShaderLibrary()->Get("PBR_Animated"), state);
    }
    {
        ShaderPipelineState state;
        state.VertexLayout = {};
        state.DepthTestEnabled = false;
        state.DepthWriteEnabled = false;

        s_Data.CompositeShaderInstance = ShaderInstance(s_Data.ShaderLibrary->Get("Composite"), state);
    }
    {
        {
            ShaderPipelineState state;
            state.VertexLayout = {};
            state.DepthTestEnabled = false;
            state.DepthWriteEnabled = false;
            //state.Samples = device->GetMaxSamples();

            s_Data.SkyboxShaderInstance = ShaderInstance(s_Data.ShaderLibrary->Get("Skybox"), state);
        }
    }

    // SHADER SYSTEMS

    // PBR systems
    ShaderSystemLayout global_layout;
    global_layout.RegisterUniformBuffer<ViewUniformBuffer>(0);
    global_layout.RegisterUniformBuffer<LightsUniformBuffer>(1);
    global_layout.RegisterCombinedTextureSampler(2); // Irradiance map
    global_layout.RegisterCombinedTextureSampler(3); // Prefilter (radiance) map
    global_layout.RegisterCombinedTextureSampler(4); // BRDF LUT

    ShaderSystemLayout mesh_layout;
    mesh_layout.RegisterInstancedUniformBuffer<MeshUniformBuffer>(0);

    ShaderSystemLayout submesh_layout;
    submesh_layout.RegisterInstancedUniformBuffer<SubmeshUniformBuffer>(0);


    s_Data.StaticGlobalSystem = ShaderSystem(global_layout, Renderer::GetShaderLibrary()->Get("PBR_Static"), 0);
    s_Data.StaticSubmeshSystem = ShaderSystem(submesh_layout, Renderer::GetShaderLibrary()->Get("PBR_Static"), 2);
    
    s_Data.AnimatedGlobalSystem = ShaderSystem(global_layout, Renderer::GetShaderLibrary()->Get("PBR_Animated"), 0);
    s_Data.AnimatedMeshSystem = ShaderSystem(mesh_layout, Renderer::GetShaderLibrary()->Get("PBR_Animated"), 2);
    s_Data.AnimatedSubmeshSystem = ShaderSystem(submesh_layout, Renderer::GetShaderLibrary()->Get("PBR_Animated"), 3);

    // Skybox system
    ShaderSystemLayout skybox_layout;
    skybox_layout.RegisterCombinedTextureSampler(0); // Skybox cube
    skybox_layout.RegisterPushConstants<SkyboxPushConstants>(ShaderStage::VertexFragment);

    s_Data.SkyboxSystem = ShaderSystem(skybox_layout, s_Data.ShaderLibrary->Get("Skybox"), 0);

    // Composite system
    ShaderSystemLayout composite_layout;
    composite_layout.RegisterCombinedTextureSampler(0);
    composite_layout.RegisterPushConstants<CompositePushConstants>(ShaderStage::Fragment);
    
    s_Data.CompositeSystem = ShaderSystem(composite_layout, s_Data.ShaderLibrary->Get("Composite"), 0);

    s_Data.BRDF = CreateBRDF();
}

void SceneRenderer::Shutdown()
{
    AR_PROFILE_FUNCTION();

    s_Data.PBRStaticShaderInstance.CleanUp();
    s_Data.PBRAnimatedShaderInstance.CleanUp();
    s_Data.SkyboxShaderInstance.CleanUp();
    s_Data.CompositeShaderInstance.CleanUp();

    s_Data.StaticGlobalSystem.Shutdown();
    s_Data.StaticSubmeshSystem.Shutdown();
    s_Data.AnimatedGlobalSystem.Shutdown();
    s_Data.AnimatedMeshSystem.Shutdown();
    s_Data.AnimatedSubmeshSystem.Shutdown();
    s_Data.SkyboxSystem.Shutdown();
    s_Data.CompositeSystem.Shutdown();

    s_Data.ShaderLibrary->CleanUp();
    delete s_Data.ShaderLibrary;

    DestroyBRDF(s_Data.BRDF);

    s_Data = {};
}

ShaderLibrary* SceneRenderer::GetShaderLibrary()
{
    return s_Data.ShaderLibrary;
}

void SceneRenderer::SetExposure(float exposure)
{
    s_Data.Exposure = exposure;
}
void SceneRenderer::SetSkyboxLOD(float lod)
{
    s_Data.SkyboxLOD = lod;
}

void SceneRenderer::SetSkybox(RenderHandle skybox_texture, RenderHandle skybox_sampler)
{
    s_Data.SkyboxSystem.SetCombinedTextureSampler(0, skybox_texture, skybox_sampler);
}

void SceneRenderer::BeginScene(PrecomputedIBLData ibl, PerspectiveCamera camera, RenderPassLayout rpl, CommandBuffer* cmd)
{
    AR_PROFILE_FUNCTION();

    s_Data.CommandBuffer = cmd;
    s_Data.SceneData.RenderPassLayout = rpl;
    s_Data.SceneData.SceneCamera = camera;
    s_Data.SceneData.SceneEnvironment = ibl;

    ViewUniformBuffer view_ub;
    view_ub.ViewProjection = camera.GetViewProjectionMatrix();
    view_ub.View = camera.GetViewMatrix();
    view_ub.CameraPos = camera.GetPosition();

    LightsUniformBuffer lights_ub;
    lights_ub.Positions[0] = vec4(-2.0f, 2.0f, 2.0f, 0.0f);
    lights_ub.Positions[1] = vec4(2.0f, 2.0f, 2.0f, 0.0f);
    lights_ub.Positions[2] = vec4(-2.0f, -2.0f, 2.0f, 0.0f);
    lights_ub.Positions[3] = vec4(2.0f, -2.0f, 2.0f, 0.0f);
    lights_ub.Colors[0] = vec4(500.0f, 0.0f, 0.0f, 0.0f);
    lights_ub.Colors[1] = vec4(0.0f, 500.0f, 0.0f, 0.0f);
    lights_ub.Colors[2] = vec4(0.0f, 300.0f, 300.0f, 0.0f);
    lights_ub.Colors[3] = vec4(300.0f, 300.0f, 0.0f, 0.0f);

    for (unsigned int i = 0; i < 4; ++i)
    {
        lights_ub.Positions[i] += vec4(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0, 1.0f);
    }

    s_Data.StaticGlobalSystem.SetUniformBuffer<ViewUniformBuffer>(0, view_ub);
    s_Data.StaticGlobalSystem.SetUniformBuffer<LightsUniformBuffer>(1, lights_ub);
    s_Data.StaticGlobalSystem.SetCombinedTextureSampler(2, ibl.IrradianceTexture, ibl.IrradianceSampler);
    s_Data.StaticGlobalSystem.SetCombinedTextureSampler(3, ibl.PrefilterEnvironmentTexture, ibl.PrefilterEnvironmentSampler);
    s_Data.StaticGlobalSystem.SetCombinedTextureSampler(4, s_Data.BRDF.Texture, s_Data.BRDF.Sampler);

    s_Data.AnimatedGlobalSystem.SetUniformBuffer<ViewUniformBuffer>(0, view_ub);
    s_Data.AnimatedGlobalSystem.SetUniformBuffer<LightsUniformBuffer>(1, lights_ub);
    s_Data.AnimatedGlobalSystem.SetCombinedTextureSampler(2, ibl.IrradianceTexture, ibl.IrradianceSampler);
    s_Data.AnimatedGlobalSystem.SetCombinedTextureSampler(3, ibl.PrefilterEnvironmentTexture, ibl.PrefilterEnvironmentSampler);
    s_Data.AnimatedGlobalSystem.SetCombinedTextureSampler(4, s_Data.BRDF.Texture, s_Data.BRDF.Sampler);

    SkyboxPushConstants sky_pc;
    sky_pc.InverseViewProjection = (camera.GetProjectionMatrix() * camera.GetViewRotationMatrix()).inverse();
    sky_pc.TextureLOD = s_Data.SkyboxLOD;
    
    // TODO: move to scene to avoid recreating this every scene
    //s_Data.SkyboxSystem.SetCombinedTextureSampler(0, ibl.PrefilterEnvironmentTexture, ibl.PrefilterEnvironmentSampler);
    s_Data.SkyboxSystem.SetPushConstants<SkyboxPushConstants>(sky_pc);
}
void SceneRenderer::EndScene()
{
    AR_PROFILE_FUNCTION();

    ScratchAllocator alloc;

    //DrawSkybox();

    s_Data.SkyboxShaderInstance.BindGraphicsPipeline(s_Data.CommandBuffer, s_Data.SceneData.RenderPassLayout);
    s_Data.SkyboxSystem.Bind(s_Data.CommandBuffer);
    s_Data.CommandBuffer->Draw(3, 1, 0, 0);


    // Draw meshes

    //s_Data.CommandBuffer->BindGraphicsPipeline(s_Data.PBRStaticShaderInstance.RequestPassPipeline(s_Data.SceneData.RenderPassLayout));
    
    //s_Data.StaticGlobalSystem.Bind(s_Data.CommandBuffer);
        
    uint32 mesh_count = s_Data.DrawList.size();
    uint32 submesh_count = 0;
    for (auto& command : s_Data.DrawList)
    {
        submesh_count += command.Mesh->GetSubmeshes().size();
    }

    MeshUniformBuffer* mesh_ub = alloc.Allocate<MeshUniformBuffer>(mesh_count);
    SubmeshUniformBuffer* submesh_ub = alloc.Allocate<SubmeshUniformBuffer>(submesh_count);

    uint32 mesh_index = 0;
    uint32 submesh_index = 0;
    for (auto& command : s_Data.DrawList)
    {
        const auto& submeshes = command.Mesh->GetSubmeshes();

        auto& bones = command.Mesh->GetBoneTransforms();
        for (uint32 i = 0; i < bones.size(); i++)
        {
            mesh_ub[mesh_index].BoneTransform[i] = bones[i];
        }
        //memcpy(&mesh_ub[mesh_index].BoneTransform, bones.data(), bones.size()); 

        for (auto& submesh : submeshes)
        {
            mat4 transform = command.Transform * submesh.Transform;
            submesh_ub[submesh_index].Transform = transform;
            submesh_ub[submesh_index].NormalTransform = transform.inverse().transpose();
            
            submesh_index++;
        }

        mesh_index++;
    }

    s_Data.AnimatedMeshSystem.SetInstancedUniformBuffer<MeshUniformBuffer>(0, mesh_count, mesh_ub);

    s_Data.StaticSubmeshSystem.SetInstancedUniformBuffer<SubmeshUniformBuffer>(0, submesh_count, submesh_ub);
    s_Data.AnimatedSubmeshSystem.SetInstancedUniformBuffer<SubmeshUniformBuffer>(0, submesh_count, submesh_ub);


    mesh_index = 0;
    submesh_index = 0;
    for (auto& command : s_Data.DrawList)
    {
        command.Mesh->Bind(s_Data.CommandBuffer);
        const auto& submeshes = command.Mesh->GetSubmeshes();

        auto systems = command.Mesh->GetMaterialSystems();

        bool is_animated = command.Mesh->IsAnimated();

        if (is_animated)
        {
            // TODO: sort by animated so only have to bind pipeline once
            s_Data.CommandBuffer->BindGraphicsPipeline(s_Data.PBRAnimatedShaderInstance.RequestPassPipeline(s_Data.SceneData.RenderPassLayout));
            s_Data.AnimatedGlobalSystem.Bind(s_Data.CommandBuffer);
            s_Data.AnimatedMeshSystem.Bind(s_Data.CommandBuffer, mesh_index);
        }
        else
        {
            s_Data.CommandBuffer->BindGraphicsPipeline(s_Data.PBRStaticShaderInstance.RequestPassPipeline(s_Data.SceneData.RenderPassLayout));
            s_Data.StaticGlobalSystem.Bind(s_Data.CommandBuffer);
        }

        for (auto submesh : submeshes)
        {
            systems[submesh.MaterialIndex]->Bind(s_Data.CommandBuffer);

            if (is_animated)
            {
                s_Data.AnimatedSubmeshSystem.Bind(s_Data.CommandBuffer, submesh_index);
            }
            else
            {
                s_Data.StaticSubmeshSystem.Bind(s_Data.CommandBuffer, submesh_index);
            }

            s_Data.CommandBuffer->DrawIndexed(submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);

            submesh_index++;
        }

        mesh_index++;
    }

    s_Data.DrawList.clear();
}

void SceneRenderer::DrawComposite(RenderHandle texture, RenderPassLayout rpl, CommandBuffer* cmd)
{
    AR_PROFILE_FUNCTION();

    s_Data.CompositeShaderInstance.BindGraphicsPipeline(cmd, rpl);

    s_Data.CompositeSystem.SetCombinedTextureSampler(0, texture, Renderer::GetWhiteSampler());
    s_Data.CompositeSystem.SetPushConstants<CompositePushConstants>({s_Data.Exposure});

    s_Data.CompositeSystem.Bind(cmd);

    cmd->Draw(3, 1, 0, 0);
}

void SceneRenderer::SubmitMesh(Mesh* mesh, const mat4& transform)
{
    AR_PROFILE_FUNCTION();

    s_Data.DrawList.push_back({mesh, transform});
}

void SceneRenderer::LoadShaders()
{
    AR_PROFILE_FUNCTION();

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
        layout.PushConstantRanges[0] = {ShaderStage::Compute, sizeof(EnvironmentPrefilterPushConstants), 0};
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
        layout.PushConstantRanges[0] = {ShaderStage::VertexFragment, sizeof(SkyboxPushConstants), 0};
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
        layout.PushConstantRanges[0] = {ShaderStage::Fragment, sizeof(CompositePushConstants), 0};
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

}

PrecomputedBRDFData SceneRenderer::CreateBRDF(uint32 size)
{
    PrecomputedBRDFData result;
    
    // Create brdf texture/sampler
    {
        TextureInfo brdf;
        brdf.Type = TextureType::Texture2D;
        brdf.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination | RenderBindFlags::TransferSource;
        brdf.Width = size;
        brdf.Height = size;
        brdf.Format = VK_FORMAT_R16G16_SFLOAT;
        brdf.MipsEnabled = true;

        SamplerInfo brdf_samp;
        brdf_samp.Filter = VK_FILTER_LINEAR;
        brdf_samp.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        brdf_samp.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        brdf_samp.Mips = MIPS(size, size);

        result.Texture = s_Data.Device->CreateTexture(brdf);
        result.Sampler = s_Data.Device->CreateSampler(brdf_samp);
    }

    RenderGraph graph(s_Data.Device);
    graph.AddPass("BRDF Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("BRDF", result.Texture, ResourceState::None());
        builder->WriteTexture("BRDF", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            RenderHandle pipeline = s_Data.ShaderLibrary->Get("BRDF")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.AddStorageImage(0, registry->GetTexture("BRDF"));
                ds0 = s_Data.ShaderLibrary->Get("BRDF")->AllocateUpdatedDescriptorSet(0, ds);
            }

            cmd->BindComputePipeline(pipeline);
            cmd->BindComputeDescriptorSet(0, ds0);
            cmd->Dispatch(size / 16, size / 16, 1);

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

        builder->ReadTexture("BRDF", ResourceState::SampledFragment());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {

        };
    });

    graph.Compile();
    graph.Evaluate();
    graph.CleanUp();

    return result;
}
void SceneRenderer::DestroyBRDF(PrecomputedBRDFData brdf)
{
    s_Data.Device->DestroyTexture(brdf.Texture);
    s_Data.Device->DestroySampler(brdf.Sampler);
}

PrecomputedIBLData SceneRenderer::CreateEnvironmentMap(const std::string& filepath)
{
    PrecomputedIBLData result;

    const uint32 cubemap_size = 2048;
    const uint32 irradiance_size = 32;

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
                ds.AddCombinedImageSampler(0, registry->GetTexture("Equirectangular"), equi_sampler);
                ds.AddStorageImage(1, registry->GetTexture("Cubemap"));
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
                ds.AddCombinedImageSampler(0, registry->GetTexture("Cubemap"), cube_sampler);
                ds.AddStorageImage(1, registry->GetTexture("Irradiance"));
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
                ds.AddCombinedImageSampler(0, registry->GetTexture("Cubemap"), cube_sampler);
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
                ds.Add(0, DescriptorType::StorageImage, 1, 0, {{registry->GetTexture("Prefilter"), RenderBindFlags::StorageTexture, RenderHandle::Null(), (int32)level}});
                RenderHandle ds1 = s_Data.ShaderLibrary->Get("EnvironmentPrefilter")->AllocateUpdatedDescriptorSet(1, ds);

                float roughness = level * deltaRoughness;

                cmd->BindComputeDescriptorSet(1, ds1);
                EnvironmentPrefilterPushConstants pc;
                pc.Roughness = roughness;
                cmd->PushConstants(ShaderStage::Compute, sizeof(pc), 0, &pc);
                cmd->Dispatch(numGroups, numGroups, 6);

                s_Data.Device->DestroyDescriptorSet(ds1);
            }

            s_Data.Device->DestroyDescriptorSet(ds0);
            s_Data.Device->DestroyComputePipeline(pipeline);
        };
    });
    graph.AddPass("Transition for Rendering", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadTexture("Irradiance", ResourceState::SampledFragment());
        builder->ReadTexture("Prefilter", ResourceState::SampledFragment());

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

PrecomputedIBLData SceneRenderer::CreateEnvironmentMap(RenderGraph* graph, const std::string& cube_name, uint32 cube_size)
{
    PrecomputedIBLData result;


    const uint32 cubemap_size = cube_size;
    const uint32 irradiance_size = 16;

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

    graph->AddPass("Generate Cubemap Mips Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Compute);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadWriteTexture(cube_name, ResourceState::TransferDestination());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->GenerateTextureMips(registry->GetTexture(cube_name));
        };
    });
    graph->AddPass("Irradiance Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Compute);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("Irradiance", result.IrradianceTexture, ResourceState::None());

        builder->ReadTexture(cube_name, ResourceState::SampledCompute());
        builder->WriteTexture("Irradiance", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            RenderHandle pipeline = s_Data.ShaderLibrary->Get("EnvironmentIrradiance")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.AddCombinedImageSampler(0, registry->GetTexture(cube_name), result.PrefilterEnvironmentSampler);
                ds.AddStorageImage(1, registry->GetTexture("Irradiance"));
                ds0 = s_Data.ShaderLibrary->Get("EnvironmentIrradiance")->AllocateUpdatedDescriptorSet(0, ds);
            }

            cmd->BindComputePipeline(pipeline);
            cmd->BindComputeDescriptorSet(0, ds0);
            cmd->Dispatch(irradiance_size / 16, irradiance_size / 16, 6);

            s_Data.Device->DestroyDescriptorSet(ds0);
            s_Data.Device->DestroyComputePipeline(pipeline);
        };
    });
    graph->AddPass("Generate Irradiance Mips Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Compute);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadWriteTexture("Irradiance", ResourceState::TransferDestination());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->GenerateTextureMips(registry->GetTexture("Irradiance"));
        };
    });
    graph->AddPass("Prefilter Copy Base Mip Level Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Compute);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("Prefilter", result.PrefilterEnvironmentTexture, ResourceState::None());

        builder->ReadTexture(cube_name, ResourceState::TransferSource());
        builder->WriteTexture("Prefilter", ResourceState::TransferDestination());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->CopyTexture(registry->GetTexture(cube_name), registry->GetTexture("Prefilter"));
        };
    });
    graph->AddPass("Prefilter Pass", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Compute);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadTexture(cube_name, ResourceState::SampledCompute());
        builder->WriteTexture("Prefilter", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            RenderHandle pipeline = s_Data.ShaderLibrary->Get("EnvironmentPrefilter")->CreateComputePipeline();
            RenderHandle ds0;
            {
                DescriptorSetUpdate ds;
                ds.AddCombinedImageSampler(0, registry->GetTexture(cube_name), result.PrefilterEnvironmentSampler);
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
                ds.Add(0, DescriptorType::StorageImage, 1, 0, {{registry->GetTexture("Prefilter"), RenderBindFlags::StorageTexture, RenderHandle::Null(), (int32)level}});
                RenderHandle ds1 = s_Data.ShaderLibrary->Get("EnvironmentPrefilter")->AllocateUpdatedDescriptorSet(1, ds);

                float roughness = level * deltaRoughness;

                cmd->BindComputeDescriptorSet(1, ds1);
                EnvironmentPrefilterPushConstants pc;
                pc.Roughness = roughness;
                cmd->PushConstants(ShaderStage::Compute, sizeof(pc), 0, &pc);
                cmd->Dispatch(numGroups, numGroups, 6);

                s_Data.Device->DestroyDescriptorSet(ds1);
            }

            s_Data.Device->DestroyDescriptorSet(ds0);
            s_Data.Device->DestroyComputePipeline(pipeline);
        };
    });
    graph->AddPass("Transition for Rendering", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadTexture("Irradiance", ResourceState::SampledFragment());
        builder->ReadTexture("Prefilter", ResourceState::SampledFragment());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {

        };
    });

    return result;
}
