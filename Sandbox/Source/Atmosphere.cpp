#include "Atmosphere.h"


#define WORKGROUP_SIZE_2D 8
#define WORKGROUP_SIZE_3D 4


void Atmosphere::Init(AtmosphereParameters params, uint32 order)
{
    m_Params = params;
    m_ScatteringOrder = order;
    
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    LoadShaders();
    // Create textures

    // Transmittance
    {
        TextureInfo info;
        info.Type = TextureType::Texture2D;
        info.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture;
        info.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
        info.Width = m_Params.transmittance_texture_mu_size;
        info.Height = m_Params.transmittance_texture_r_size;

        m_TransmittanceTexture = device->CreateTexture(info);
    }
    // Irradiance
    {
        TextureInfo info;
        info.Type = TextureType::Texture2D;
        info.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture;
        info.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
        info.Width = m_Params.irradiance_texture_mu_s_size;
        info.Height = m_Params.irradiance_texture_r_size;

        m_DeltaIrradianceTexture = device->CreateTexture(info);
        info.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination;
        m_IrradianceTexture = device->CreateTexture(info);
        info.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::TransferSource;
        m_IrradianceReadTexture = device->CreateTexture(info);
    }
    // Scattering
    {
        TextureInfo info;
        info.Type = TextureType::Texture3D;
        info.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture;
        info.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
        info.Width = m_Params.scattering_texture_nu_size * m_Params.scattering_texture_mu_s_size;
        info.Height = m_Params.scattering_texture_mu_size;
        info.Depth = m_Params.scattering_texture_r_size;

        m_DeltaRayleighTexture = device->CreateTexture(info);
        m_DeltaMieTexture = device->CreateTexture(info);
        m_ScatteringDensityTexture = device->CreateTexture(info);
        // We use a render pass to clear initial
        info.BindFlags |= RenderBindFlags::ColorAttachment;
        m_DeltaMultipleScatteringTexture = device->CreateTexture(info);

        info.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination;
        m_ScatteringTexture = device->CreateTexture(info);
        info.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::TransferSource;
        m_ScatteringReadTexture = device->CreateTexture(info);
    }
    // Linear sampler
    {
        SamplerInfo info;
        info.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.Filter = VK_FILTER_LINEAR;
        info.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        m_Sampler = device->CreateSampler(info);
    }

    // Pipelines

    m_TransmittanceShaderInstance = ShaderInstance(m_ShaderLibrary.Get("Transmittance"));
    m_DirectIrradianceShaderInstance = ShaderInstance(m_ShaderLibrary.Get("DirectIrradiance"));
    m_SingleScatteringShaderInstance = ShaderInstance(m_ShaderLibrary.Get("SingleScattering"));
    m_ScatteringDensityShaderInstance = ShaderInstance(m_ShaderLibrary.Get("ScatteringDensity"));
    m_IndirectIrradianceShaderInstance = ShaderInstance(m_ShaderLibrary.Get("IndirectIrradiance"));
    m_MultipleScatteringShaderInstance = ShaderInstance(m_ShaderLibrary.Get("MultipleScattering"));

    // Set uniforms

    m_TransmittanceShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_TransmittanceShaderSystem.SetStorageTexture(1, m_TransmittanceTexture);

    m_DirectIrradianceShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_DirectIrradianceShaderSystem.SetCombinedTextureSampler(1, m_TransmittanceTexture, m_Sampler);
    m_DirectIrradianceShaderSystem.SetStorageTexture(2, m_DeltaIrradianceTexture);

    m_SingleScatteringShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_SingleScatteringShaderSystem.SetCombinedTextureSampler(1, m_TransmittanceTexture, m_Sampler);
    m_SingleScatteringShaderSystem.SetStorageTexture(2, m_DeltaRayleighTexture);
    m_SingleScatteringShaderSystem.SetStorageTexture(3, m_DeltaMieTexture);
    m_SingleScatteringShaderSystem.SetStorageTexture(4, m_ScatteringTexture);

    m_ScatteringDensityShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_ScatteringDensityShaderSystem.SetCombinedTextureSampler(1, m_TransmittanceTexture, m_Sampler);
    m_ScatteringDensityShaderSystem.SetCombinedTextureSampler(2, m_DeltaRayleighTexture, m_Sampler);
    m_ScatteringDensityShaderSystem.SetCombinedTextureSampler(3, m_DeltaMieTexture, m_Sampler);
    m_ScatteringDensityShaderSystem.SetCombinedTextureSampler(4, m_DeltaMultipleScatteringTexture, m_Sampler);
    m_ScatteringDensityShaderSystem.SetCombinedTextureSampler(5, m_DeltaIrradianceTexture, m_Sampler);
    m_ScatteringDensityShaderSystem.SetStorageTexture(6, m_ScatteringDensityTexture);

    m_IndirectIrradianceShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_IndirectIrradianceShaderSystem.SetCombinedTextureSampler(1, m_DeltaRayleighTexture, m_Sampler);
    m_IndirectIrradianceShaderSystem.SetCombinedTextureSampler(2, m_DeltaMieTexture, m_Sampler);
    m_IndirectIrradianceShaderSystem.SetCombinedTextureSampler(3, m_DeltaMultipleScatteringTexture, m_Sampler);
    m_IndirectIrradianceShaderSystem.SetStorageTexture(4, m_DeltaIrradianceTexture);
    m_IndirectIrradianceShaderSystem.SetStorageTexture(5, m_IrradianceReadTexture);
    m_IndirectIrradianceShaderSystem.SetStorageTexture(6, m_IrradianceTexture);

    m_MultipleScatteringShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_MultipleScatteringShaderSystem.SetCombinedTextureSampler(1, m_TransmittanceTexture, m_Sampler);
    m_MultipleScatteringShaderSystem.SetCombinedTextureSampler(2, m_ScatteringDensityTexture, m_Sampler);
    m_MultipleScatteringShaderSystem.SetStorageTexture(3, m_DeltaMultipleScatteringTexture);
    m_MultipleScatteringShaderSystem.SetStorageTexture(4, m_ScatteringReadTexture);
    m_MultipleScatteringShaderSystem.SetStorageTexture(5, m_ScatteringTexture);

    Update(m_Params, m_ScatteringOrder);
}
void Atmosphere::Shutdown()
{
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    device->DestroyTexture(m_TransmittanceTexture);
    device->DestroyTexture(m_DeltaIrradianceTexture);
    device->DestroyTexture(m_DeltaRayleighTexture);
    device->DestroyTexture(m_DeltaMieTexture);
    device->DestroyTexture(m_ScatteringTexture);
    device->DestroyTexture(m_DeltaMultipleScatteringTexture);
    device->DestroyTexture(m_ScatteringDensityTexture);
    device->DestroyTexture(m_IrradianceTexture);
    device->DestroyTexture(m_IrradianceReadTexture);
    device->DestroyTexture(m_ScatteringReadTexture);
    device->DestroySampler(m_Sampler);

    m_TransmittanceShaderInstance.CleanUp();
    m_DirectIrradianceShaderInstance.CleanUp();
    m_SingleScatteringShaderInstance.CleanUp();
    m_ScatteringDensityShaderInstance.CleanUp();
    m_IndirectIrradianceShaderInstance.CleanUp();
    m_MultipleScatteringShaderInstance.CleanUp();

    m_TransmittanceShaderSystem.Shutdown();
    m_DirectIrradianceShaderSystem.Shutdown();
    m_SingleScatteringShaderSystem.Shutdown();
    m_ScatteringDensityShaderSystem.Shutdown();
    m_IndirectIrradianceShaderSystem.Shutdown();
    m_MultipleScatteringShaderSystem.Shutdown();

    m_ShaderLibrary.CleanUp();
}

void Atmosphere::Update(AtmosphereParameters params, uint32 order)
{
    m_Params = params;
    m_ScatteringOrder = order;

    // Set atmosphere parameters uniform
    m_TransmittanceShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_DirectIrradianceShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_SingleScatteringShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_ScatteringDensityShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_IndirectIrradianceShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);
    m_MultipleScatteringShaderSystem.SetUniformBuffer<AtmosphereParameters>(0, m_Params);

    Precompute();
}

void Atmosphere::Precompute()
{
    RenderGraph graph(Application::Get()->GetRenderBackend()->GetDevice());

    graph.AddPass("Transmittance", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("Transmittance", m_TransmittanceTexture);
        builder->WriteTexture("Transmittance", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            const TextureInfo& info = registry->GetTextureInfo("Transmittance");

            m_TransmittanceShaderInstance.BindComputePipeline(cmd);
            m_TransmittanceShaderSystem.BindCompute(cmd);
            cmd->Dispatch(info.Width / WORKGROUP_SIZE_2D, info.Height / WORKGROUP_SIZE_2D, 1);
        };
    });
    graph.AddPass("DirectIrradiance", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("DeltaIrradiance", m_DeltaIrradianceTexture);

        builder->ReadTexture("Transmittance", ResourceState::SampledCompute());
        builder->WriteTexture("DeltaIrradiance", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            const TextureInfo& info = registry->GetTextureInfo("DeltaIrradiance");

            m_DirectIrradianceShaderInstance.BindComputePipeline(cmd);
            m_DirectIrradianceShaderSystem.BindCompute(cmd);
            cmd->Dispatch(info.Width / WORKGROUP_SIZE_2D, info.Height / WORKGROUP_SIZE_2D, 1);
        };
    });
    graph.AddPass("SingleScattering", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("DeltaRayleigh", m_DeltaRayleighTexture);
        builder->ImportTexture("DeltaMie", m_DeltaMieTexture);
        builder->ImportTexture("Scattering", m_ScatteringTexture);

        builder->ReadTexture("Transmittance", ResourceState::SampledCompute());
        builder->WriteTexture("DeltaRayleigh", ResourceState::StorageTextureCompute());
        builder->WriteTexture("DeltaMie", ResourceState::StorageTextureCompute());
        builder->WriteTexture("Scattering", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            const TextureInfo& info = registry->GetTextureInfo("Scattering");

            m_SingleScatteringShaderInstance.BindComputePipeline(cmd);
            m_SingleScatteringShaderSystem.BindCompute(cmd);
            cmd->Dispatch(info.Width / WORKGROUP_SIZE_3D, info.Height / WORKGROUP_SIZE_3D, info.Depth / WORKGROUP_SIZE_3D);
        };
    });
    // Empty render graph render pass to
    // 1. import Irradiance, DeltaMultipleScattering and ScatteringDensity textures
    // 2. clear DeltaMultipleScattering
    graph.AddPass("Transition for Multiple Scattering", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Render);

        builder->ImportTexture("Irradiance", m_IrradianceTexture);
        builder->ImportTexture("IrradianceRead", m_IrradianceReadTexture);
        builder->ImportTexture("ScatteringRead", m_ScatteringReadTexture);
        builder->ImportTexture("DeltaMultipleScattering", m_DeltaMultipleScatteringTexture);
        builder->ImportTexture("ScatteringDensity", m_ScatteringDensityTexture);

        // Clear to default (0, 0, 0, 0)
        builder->WriteTexture("DeltaMultipleScattering", ResourceState::Color(), true);

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {

        };
    });
    // Calculate higher scattering orders
    for (int32 i = 2; i < m_ScatteringOrder; i++)
    {
        graph.AddPass("ScatteringDensity " + std::to_string(i), [&](RenderGraphBuilder* builder) {
            builder->SetQueue(QueueType::Universal);
            builder->SetType(RenderGraphPassType::Other);

            builder->ReadTexture("Transmittance", ResourceState::SampledCompute());
            builder->ReadTexture("DeltaRayleigh", ResourceState::SampledCompute());
            builder->ReadTexture("DeltaMie", ResourceState::SampledCompute());
            builder->ReadTexture("DeltaMultipleScattering", ResourceState::SampledCompute());
            builder->ReadTexture("DeltaIrradiance", ResourceState::SampledCompute());
            builder->WriteTexture("ScatteringDensity", ResourceState::StorageTextureCompute());

            return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
                const TextureInfo& info = registry->GetTextureInfo("ScatteringDensity");

                m_ScatteringDensityShaderSystem.SetPushConstants<int>(i);
                m_ScatteringDensityShaderInstance.BindComputePipeline(cmd);
                m_ScatteringDensityShaderSystem.BindCompute(cmd);
                cmd->Dispatch(info.Width / WORKGROUP_SIZE_3D, info.Height / WORKGROUP_SIZE_3D, info.Depth / WORKGROUP_SIZE_3D);
            };
        });
        graph.AddPass("Copy Irradiance and Scattering " + std::to_string(i), [&](RenderGraphBuilder* builder) {
            builder->SetQueue(QueueType::Universal);
            builder->SetType(RenderGraphPassType::Other);

            builder->ReadTexture("Irradiance", ResourceState::TransferSource());
            builder->ReadTexture("Scattering", ResourceState::TransferSource());
            builder->WriteTexture("IrradianceRead", ResourceState::TransferDestination());
            builder->WriteTexture("ScatteringRead", ResourceState::TransferDestination());

            return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
                cmd->CopyTexture(registry->GetTexture("Irradiance"), registry->GetTexture("IrradianceRead"));
                cmd->CopyTexture(registry->GetTexture("Scattering"), registry->GetTexture("ScatteringRead"));
            };
        });
        graph.AddPass("IndirectIrradiance " + std::to_string(i), [&](RenderGraphBuilder* builder) {
            builder->SetQueue(QueueType::Universal);
            builder->SetType(RenderGraphPassType::Other);

            builder->ReadTexture("DeltaRayleigh", ResourceState::SampledCompute());
            builder->ReadTexture("DeltaMie", ResourceState::SampledCompute());
            builder->ReadTexture("DeltaMultipleScattering", ResourceState::SampledCompute());
            builder->ReadTexture("IrradianceRead", ResourceState::StorageTextureCompute());
            builder->WriteTexture("DeltaIrradiance", ResourceState::StorageTextureCompute());
            builder->WriteTexture("Irradiance", ResourceState::StorageTextureCompute());

            return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
                const TextureInfo& info = registry->GetTextureInfo("Irradiance");

                m_IndirectIrradianceShaderSystem.SetPushConstants<int>(i);
                m_IndirectIrradianceShaderInstance.BindComputePipeline(cmd);
                m_IndirectIrradianceShaderSystem.BindCompute(cmd);
                cmd->Dispatch(info.Width / WORKGROUP_SIZE_2D, info.Height / WORKGROUP_SIZE_2D, 1);
            };
        });
        graph.AddPass("MultipleScattering " + std::to_string(i), [&](RenderGraphBuilder* builder) {
            builder->SetQueue(QueueType::Universal);
            builder->SetType(RenderGraphPassType::Other);

            builder->ReadTexture("Transmittance", ResourceState::SampledCompute());
            builder->ReadTexture("ScatteringDensity", ResourceState::SampledCompute());
            builder->ReadTexture("ScatteringRead", ResourceState::StorageTextureCompute());
            builder->WriteTexture("DeltaMultipleScattering", ResourceState::StorageTextureCompute());
            builder->WriteTexture("Scattering", ResourceState::StorageTextureCompute());

            return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
                const TextureInfo& info = registry->GetTextureInfo("Scattering");

                m_MultipleScatteringShaderInstance.BindComputePipeline(cmd);
                m_MultipleScatteringShaderSystem.BindCompute(cmd);
                cmd->Dispatch(info.Width / WORKGROUP_SIZE_3D, info.Height / WORKGROUP_SIZE_3D, info.Depth / WORKGROUP_SIZE_3D);
            };
        });
    }

    graph.AddPass("Transition for Rendering", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadTexture("Transmittance", ResourceState::SampledFragment());
        builder->ReadTexture("Irradiance", ResourceState::SampledFragment());
        builder->ReadTexture("Scattering", ResourceState::SampledFragment());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {

        };
    });
    graph.Compile();
    graph.Evaluate();

    m_PrecomputeStats = graph.GetStats();

    graph.CleanUp();

    m_NeedsPrecompute = false;
}

void Atmosphere::LoadShaders()
{
    m_ShaderLibrary = ShaderLibrary(Application::Get()->GetRenderBackend()->GetDevice());

    // Transmittance
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 0;
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Compute},
            {1, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };

        m_ShaderLibrary.Load("Transmittance", "Assets/Shaders/Atmosphere/Transmittance.comp.spv", layout);
    }
    {
        ShaderSystemLayout layout;
        layout.RegisterUniformBuffer<AtmosphereParameters>(0);
        layout.RegisterStorageTexture(1);

        m_TransmittanceShaderSystem = ShaderSystem(layout, m_ShaderLibrary.Get("Transmittance"), 0);
    }
    // Direct irradiance
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 0;
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Compute},
            {1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {2, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };

        m_ShaderLibrary.Load("DirectIrradiance", "Assets/Shaders/Atmosphere/DirectIrradiance.comp.spv", layout);
    }
    {
        ShaderSystemLayout layout;
        layout.RegisterUniformBuffer<AtmosphereParameters>(0);
        layout.RegisterCombinedTextureSampler(1);
        layout.RegisterStorageTexture(2);

        m_DirectIrradianceShaderSystem = ShaderSystem(layout, m_ShaderLibrary.Get("DirectIrradiance"), 0);
    }
    // Single scattering
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 0;
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Compute},
            {1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {2, DescriptorType::StorageImage, 1, ShaderStage::Compute},
            {3, DescriptorType::StorageImage, 1, ShaderStage::Compute},
            {4, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };

        m_ShaderLibrary.Load("SingleScattering", "Assets/Shaders/Atmosphere/SingleScattering.comp.spv", layout);
    }
    {
        ShaderSystemLayout layout;
        layout.RegisterUniformBuffer<AtmosphereParameters>(0);
        layout.RegisterCombinedTextureSampler(1);
        layout.RegisterStorageTexture(2);
        layout.RegisterStorageTexture(3);
        layout.RegisterStorageTexture(4);

        m_SingleScatteringShaderSystem = ShaderSystem(layout, m_ShaderLibrary.Get("SingleScattering"), 0);
    }
    // Scattering density
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {
            ShaderStage::Compute, sizeof(int), 0};
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Compute},
            {1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {2, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {3, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {4, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {5, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {6, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };

        m_ShaderLibrary.Load("ScatteringDensity", "Assets/Shaders/Atmosphere/ScatteringDensity.comp.spv", layout);
    }
    {
        ShaderSystemLayout layout;
        layout.RegisterUniformBuffer<AtmosphereParameters>(0);
        layout.RegisterCombinedTextureSampler(1);
        layout.RegisterCombinedTextureSampler(2);
        layout.RegisterCombinedTextureSampler(3);
        layout.RegisterCombinedTextureSampler(4);
        layout.RegisterCombinedTextureSampler(5);
        layout.RegisterStorageTexture(6);
        layout.RegisterPushConstants<int>(ShaderStage::Compute);

        m_ScatteringDensityShaderSystem = ShaderSystem(layout, m_ShaderLibrary.Get("ScatteringDensity"), 0);
    }
    // Indirect irradiance
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {
            ShaderStage::Compute, sizeof(int), 0};
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Compute},
            {1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {2, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {3, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {4, DescriptorType::StorageImage, 1, ShaderStage::Compute},
            {5, DescriptorType::StorageImage, 1, ShaderStage::Compute},
            {6, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };

        m_ShaderLibrary.Load("IndirectIrradiance", "Assets/Shaders/Atmosphere/IndirectIrradiance.comp.spv", layout);
    }
    {
        ShaderSystemLayout layout;
        layout.RegisterUniformBuffer<AtmosphereParameters>(0);
        layout.RegisterCombinedTextureSampler(1);
        layout.RegisterCombinedTextureSampler(2);
        layout.RegisterCombinedTextureSampler(3);
        layout.RegisterStorageTexture(4);
        layout.RegisterStorageTexture(5);
        layout.RegisterStorageTexture(6);
        layout.RegisterPushConstants<int>(ShaderStage::Compute);

        m_IndirectIrradianceShaderSystem = ShaderSystem(layout, m_ShaderLibrary.Get("IndirectIrradiance"), 0);
    }
    // Multiple scattering
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 0;
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Compute},
            {1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {2, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
            {3, DescriptorType::StorageImage, 1, ShaderStage::Compute},
            {4, DescriptorType::StorageImage, 1, ShaderStage::Compute},
            {5, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };

        m_ShaderLibrary.Load("MultipleScattering", "Assets/Shaders/Atmosphere/MultipleScattering.comp.spv", layout);
    }
    {
        ShaderSystemLayout layout;
        layout.RegisterUniformBuffer<AtmosphereParameters>(0);
        layout.RegisterCombinedTextureSampler(1);
        layout.RegisterCombinedTextureSampler(2);
        layout.RegisterStorageTexture(3);
        layout.RegisterStorageTexture(4);
        layout.RegisterStorageTexture(5);

        m_MultipleScatteringShaderSystem = ShaderSystem(layout, m_ShaderLibrary.Get("MultipleScattering"), 0);
    }
}