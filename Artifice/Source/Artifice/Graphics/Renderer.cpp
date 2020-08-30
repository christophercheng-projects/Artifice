#include "Renderer.h"

#include "Renderer2D.h"
#include "SceneRenderer.h"

#include "Resources.h"

struct RendererData
{
    Device* Device;
    ShaderLibrary* ShaderLibrary;
    RenderPassLayout ActiveRenderPassLayout;

    RenderHandle WhiteTexture;
    RenderHandle WhiteSampler;
    
    RenderHandle LinearSampler;
};

static RendererData s_Data;

void Renderer::Init(Device* device)
{
    s_Data.Device = device;

    s_Data.ShaderLibrary = new ShaderLibrary(device);

    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {ShaderStage::VertexFragment, sizeof(PBRPushConstants), 0};
        layout.DescriptorSetLayoutCount = 3;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Vertex},   // viewprojection, cameraposition
            {1, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Fragment}, // lights
            {2, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // irradiance map
            {3, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // prefilter map
            {4, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // brdf lut
        };
        layout.DescriptorSetLayouts[1] = {
            {0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // Albedo
            {1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // Normal
            {2, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // Metalness
            {3, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // Roughness
        };
        layout.DescriptorSetLayouts[2] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Vertex}, // transform, normaltransform
        };

        s_Data.ShaderLibrary->Load("PBR_Static", "Assets/Shaders/PBR_Static.vert.spv", "Assets/Shaders/PBR_Static.frag.spv", layout);
    }
    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {ShaderStage::VertexFragment, sizeof(PBRPushConstants), 0};
        layout.DescriptorSetLayoutCount = 4;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Vertex},   // viewprojection, cameraposition
            {1, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Fragment}, // lights
            {2, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // irradiance map
            {3, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // prefilter map
            {4, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // brdf lut
        };
        layout.DescriptorSetLayouts[1] = {
            {0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // Albedo
            {1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // Normal
            {2, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // Metalness
            {3, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}, // Roughness
        };
        layout.DescriptorSetLayouts[2] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Vertex}, // bone transform
        };
        layout.DescriptorSetLayouts[3] = {
            {0, DescriptorType::UniformBufferDynamic, 1, ShaderStage::Vertex}, // transform, normaltransform
        };

        s_Data.ShaderLibrary->Load("PBR_Animated", "Assets/Shaders/PBR_Animated.vert.spv", "Assets/Shaders/PBR_Animated.frag.spv", layout);
    }

    SamplerInfo sampler;
    sampler.Filter = VK_FILTER_NEAREST;
    sampler.AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.Mips = 1;

    s_Data.WhiteSampler = device->CreateSampler(sampler);

    sampler.Filter = VK_FILTER_LINEAR;
    sampler.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.Mips = 1;

    s_Data.LinearSampler = device->CreateSampler(sampler);

    TextureInfo tex;
    tex.MipsEnabled = false;
    tex.Width = 1;
    tex.Height = 1;
    tex.Format = VK_FORMAT_R8G8B8A8_UNORM;
    tex.Type = TextureType::Texture2D;
    tex.BindFlags = RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination;

    s_Data.WhiteTexture = device->CreateTexture(tex);
    uint32 tex_data = 0xffffffff;

    CommandBuffer cmd = device->RequestCommandBuffer(QueueType::Universal);
    cmd.Begin();
    cmd.UploadTextureData(s_Data.WhiteTexture, sizeof(uint32), &tex_data, false, ResourceState::SampledFragment());
    cmd.End();
    device->CommitCommandBuffer(cmd);
    device->SubmitQueue(QueueType::Universal, {}, {}, true);

    Renderer2D::Init(device);
    SceneRenderer::Init(device);
}

void Renderer::Shutdown()
{
    SceneRenderer::Shutdown();
    Renderer2D::Shutdown();

    s_Data.ShaderLibrary->CleanUp();
    s_Data.Device->DestroyTexture(s_Data.WhiteTexture);
    s_Data.Device->DestroySampler(s_Data.WhiteSampler);
    s_Data.Device->DestroySampler(s_Data.LinearSampler);
    delete s_Data.ShaderLibrary;
    s_Data = {};
}

ShaderLibrary* Renderer::GetShaderLibrary()
{
    return s_Data.ShaderLibrary;
}

RenderHandle Renderer::GetWhiteTexture()
{
    return s_Data.WhiteTexture;
}
RenderHandle Renderer::GetWhiteSampler()
{
    return s_Data.WhiteSampler;
}
RenderHandle Renderer::GetLinearSampler()
{
    return s_Data.LinearSampler;
}