#include "AtmosphereLayer.h"

#include <imgui.h>
#include <stb_image.h>
#if 0
#define TRANSMITTANCE_WIDTH 64
#define TRANSMITTANCE_HEIGHT 256
#define SCATTERING_WIDTH 64
#define SCATTERING_HEIGHT 256
#define SCATTERING_DEPTH 128

#define TRANSMITTANCE_WORKGROUP_SIZE 32
#define SCATTERING_WORKGROUP_SIZE 8

AtmosphereLayer::AtmosphereLayer()
{

}

void AtmosphereLayer::OnAttach()
{
    AR_PROFILE_FUNCTION();

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    TextureInfo transmittance;
    transmittance.Type = TextureType::Texture2D;
    transmittance.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture;
    transmittance.Width = TRANSMITTANCE_WIDTH;
    transmittance.Height = TRANSMITTANCE_HEIGHT;
    transmittance.Format = VK_FORMAT_R16G16B16A16_SFLOAT;

    TextureInfo scattering;
    scattering.Type = TextureType::Texture3D;
    scattering.BindFlags = RenderBindFlags::StorageTexture | RenderBindFlags::SampledTexture;
    scattering.Width = SCATTERING_WIDTH;
    scattering.Height = SCATTERING_HEIGHT;
    scattering.Depth = SCATTERING_DEPTH;
    scattering.Format = VK_FORMAT_R16G16B16A16_SFLOAT;

    m_TransmittanceTexture = device->CreateTexture(transmittance);
    m_ScatteringTexture = device->CreateTexture(scattering);

    SamplerInfo sampler;
    sampler.Filter = VK_FILTER_LINEAR;
    sampler.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.Mips = 1;

    m_TransmittanceComputeSampler = device->CreateSampler(sampler);


    Shader::Layout transmittance_layout;
    transmittance_layout.PushConstantRangeCount = 0;
    transmittance_layout.DescriptorSetLayoutCount = 1;
    transmittance_layout.DescriptorSetLayouts[0] = {
        {0, DescriptorType::StorageImage, 1, ShaderStage::Compute},
    };

    Shader::Layout scattering_layout;
    scattering_layout.PushConstantRangeCount = 0;
    scattering_layout.DescriptorSetLayoutCount = 1;
    scattering_layout.DescriptorSetLayouts[0] = {
        {0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute},
        {1, DescriptorType::StorageImage, 1, ShaderStage::Compute},
    };
    
    m_TransmittanceShader = Shader(device, "Shaders/Transmittance.comp.spv", transmittance_layout);
    m_ScatteringShader = Shader(device, "Shaders/Scattering.comp.spv", scattering_layout);

    m_TransmittanceComputeMaterial = Material(&m_TransmittanceShader);
    m_ScatteringComputeMaterial = Material(&m_ScatteringShader);
    
    {
        DescriptorSetUpdate ds;
        ds.Add(0, DescriptorType::StorageImage, 1, 0, {{m_TransmittanceTexture, ResourceState::StorageTextureCompute(), RenderHandle()}});
        m_TransmittanceComputeMaterial.UpdateDescriptorSet(0, ds);
    }
    {
        DescriptorSetUpdate ds;
        ds.Add(0, DescriptorType::CombinedImageSampler, 1, 0, {{m_TransmittanceTexture, ResourceState::SampledCompute(), m_TransmittanceComputeSampler}});
        ds.Add(1, DescriptorType::StorageImage, 1, 0, {{m_ScatteringTexture, ResourceState::StorageTextureCompute(), RenderHandle()}});
        m_ScatteringComputeMaterial.UpdateDescriptorSet(0, ds);
    }

    m_PrecomputeGraph = new RenderGraph(device);
    m_PrecomputeGraph->AddPass("Transmittance Pass", [&](RenderGraphBuilder* builder){
        
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);
        
        builder->ImportTexture("Transmittance", m_TransmittanceTexture, ResourceState::None());
        builder->WriteTexture("Transmittance", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd){
            
            cmd->BindComputePipeline(m_TransmittanceComputeMaterial.GetPipeline());
            cmd->BindComputeDescriptorSet(0, m_TransmittanceComputeMaterial.GetDescriptorSet(0));

            cmd->Dispatch(TRANSMITTANCE_WIDTH / TRANSMITTANCE_WORKGROUP_SIZE, TRANSMITTANCE_HEIGHT / TRANSMITTANCE_WORKGROUP_SIZE, 1);
        };
    });
    m_PrecomputeGraph->AddPass("Scattering Pass", [&](RenderGraphBuilder* builder){
        
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("Scattering", m_ScatteringTexture, ResourceState::None());
        
        builder->ReadTexture("Transmittance", ResourceState::SampledCompute());
        builder->WriteTexture("Scattering", ResourceState::StorageTextureCompute());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            
            cmd->BindComputePipeline(m_ScatteringComputeMaterial.GetPipeline());
            cmd->BindComputeDescriptorSet(0, m_ScatteringComputeMaterial.GetDescriptorSet(0));

            cmd->Dispatch(SCATTERING_WIDTH / SCATTERING_WORKGROUP_SIZE, SCATTERING_HEIGHT / SCATTERING_WORKGROUP_SIZE, SCATTERING_DEPTH / SCATTERING_WORKGROUP_SIZE);
        };
    });
    m_PrecomputeGraph->AddPass("Transition Pass", [&](RenderGraphBuilder* builder){
        
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ReadTexture("Transmittance", ResourceState::SampledFragment());
        builder->ReadTexture("Scattering", ResourceState::SampledFragment());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            
        };
    });

    m_PrecomputeGraph->Compile();
    m_PrecomputeGraph->Evaluate();
}

void AtmosphereLayer::OnDetach()
{
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    
    m_TransmittanceComputeMaterial.CleanUp();
    m_ScatteringComputeMaterial.CleanUp();
    m_TransmittanceShader.CleanUp();
    m_ScatteringShader.CleanUp();

    
    device->DestroySampler(m_TransmittanceComputeSampler);
    
    device->DestroyTexture(m_ScatteringTexture);
    device->DestroyTexture(m_TransmittanceTexture);
}

void AtmosphereLayer::OnConstructRenderGraph(RenderGraph* graph)
{
    AR_PROFILE_FUNCTION();

   graph->AddPass("Renderer2D Transmittance", [&](RenderGraphBuilder* builder) {
        
        builder->ReadWriteTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Color());
        
        TextureInfo color = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);

        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            
            cmd->SetViewport(color.Width, color.Height);
            cmd->SetScissor(0, 0, color.Width, color.Height);
            

            Renderer2D::BeginScene(mat4::identity(), rpl, cmd);
            
            Renderer2D::DrawQuad({0.0f, 0.0f, 0.5f}, {1.0f, 1.0f}, m_TransmittanceTexture, 1.0f);

            Renderer2D::EndScene();

        };
    });

}

void AtmosphereLayer::OnUpdate(Timestep ts)
{
    AR_PROFILE_FUNCTION();

}

void AtmosphereLayer::OnImGuiRender()
{

}

void AtmosphereLayer::OnEvent(Event& e)
{

}

#endif