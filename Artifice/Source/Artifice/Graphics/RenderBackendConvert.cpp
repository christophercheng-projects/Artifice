#include "RenderBackend.h"

#include <vulkan/vulkan.h>


VkAttachmentLoadOp RenderBackend::ConvertAttachmentLoadOptions(LoadOptions options)
{
    switch (options)
    {
    case LoadOptions::Discard:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    case LoadOptions::Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadOptions::Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;

    default:
        AR_FATAL("");
    }
}

VkAttachmentStoreOp RenderBackend::ConvertAttachmentStoreOptions(StoreOptions options)
{
    switch (options)
    {
    case StoreOptions::Discard:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    case StoreOptions::Store:
        return VK_ATTACHMENT_STORE_OP_STORE;
    default:
        AR_FATAL("");
    }
}

VkFormat RenderBackend::ConvertFormat(Format format)
{
    switch (format)
    {
    case Format::R32Float:
        return VK_FORMAT_R32_SFLOAT;
    case Format::RG32Float:
        return VK_FORMAT_R32G32_SFLOAT;
    case Format::RGB32Float:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case Format::RGBA32Float:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case Format::RGBA8Unorm:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case Format::BGRA8Unorm:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case Format::RGBA32Int:
        return VK_FORMAT_R32G32B32A32_SINT;
    default:
        AR_FATAL("");
    }
}

VulkanResourceState RenderBackend::ConvertResourceState(ResourceState state, bool zero_access_reads)
{
    VulkanResourceState result;
    result.Access = ConvertRenderBindFlagsToAccess(state.Bind, zero_access_reads);
    result.Stage = ConvertPipelineStageFlags(state.Stage);
    result.ImageLayout = ConvertRenderBindFlagToImageLayout(state.Bind);

    return result;
}

VkDescriptorType RenderBackend::ConvertDescriptorType(DescriptorType type)
{
    switch (type)
    {
    case DescriptorType::StorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case DescriptorType::StorageBufferDynamic:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    case DescriptorType::StorageImage:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case DescriptorType::UniformBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::UniformBufferDynamic:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case DescriptorType::CombinedImageSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default:
        AR_CORE_FATAL("Invalid descriptor type.");
    }
}
VkShaderStageFlags RenderBackend::ConvertShaderStage(ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::Compute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case ShaderStage::Vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::Fragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::VertexFragment:
        return VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    default:
        AR_CORE_FATAL("Invalid shader stage.");
    }
}

VkAccessFlags RenderBackend::ConvertRenderBindFlagsToAccess(RenderBindFlags flags, bool zero_reads)
{
    VkAccessFlags result = 0;
    if (!zero_reads)
    {
        if (Contains(flags, RenderBindFlags::TransferSource))
        {
            result |= VK_ACCESS_TRANSFER_READ_BIT;
        }
        if (Contains(flags, RenderBindFlags::UniformBuffer))
        {
            result |= VK_ACCESS_SHADER_READ_BIT;
        }
        if (Contains(flags, RenderBindFlags::VertexBuffer))
        {
            result |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        }
        if (Contains(flags, RenderBindFlags::IndexBuffer))
        {
            result |= VK_ACCESS_INDEX_READ_BIT;
        }
        if (Contains(flags, RenderBindFlags::InputAttachment))
        {
            result |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        }
        if (Contains(flags, RenderBindFlags::SampledTexture))
        {
            result |= VK_ACCESS_SHADER_READ_BIT;
        }
    }
    if (Contains(flags, RenderBindFlags::TransferDestination))
    {
        result |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if (Contains(flags, RenderBindFlags::StorageBuffer))
    {
        result |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }
    if (Contains(flags, RenderBindFlags::ColorAttachment))
    {
        result |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (Contains(flags, RenderBindFlags::DepthStencilAttachment))
    {
        result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    if (Contains(flags, RenderBindFlags::ResolveAttachment))
    {
        result |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (Contains(flags, RenderBindFlags::StorageTexture))
    {
        result |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }

    return result;
}

VkPipelineStageFlags RenderBackend::ConvertPipelineStageFlags(PipelineStageFlags flags)
{
    if (Contains(flags, PipelineStageFlags::All))
    {
        return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    
    VkPipelineStageFlags result = 0;
    if (Contains(flags, PipelineStageFlags::Transfer))
    {
        result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    if (Contains(flags, PipelineStageFlags::VertexInput))
    {
        result |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    }
    if (Contains(flags, PipelineStageFlags::Vertex))
    {
        result |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    }
    if (Contains(flags, PipelineStageFlags::Fragment))
    {
        result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    if (Contains(flags, PipelineStageFlags::Compute))
    {
        result |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    if (Contains(flags, PipelineStageFlags::Color))
    {
        result |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    if (Contains(flags, PipelineStageFlags::DepthStencil))
    {
        result |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }

    return result;
}

VkImageLayout RenderBackend::ConvertRenderBindFlagToImageLayout(RenderBindFlags flag)
{
    switch (flag)
    {
    case RenderBindFlags::None:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case RenderBindFlags::TransferSource:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case RenderBindFlags::TransferDestination:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case RenderBindFlags::ColorAttachment:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RenderBindFlags::ResolveAttachment:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RenderBindFlags::DepthStencilAttachment:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case RenderBindFlags::InputAttachment:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case RenderBindFlags::SampledTexture:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case RenderBindFlags::StorageTexture:
        return VK_IMAGE_LAYOUT_GENERAL;
    case RenderBindFlags::Present:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
        AR_CORE_FATAL("");
    }
}