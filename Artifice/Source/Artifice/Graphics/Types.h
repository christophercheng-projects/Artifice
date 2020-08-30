#pragma once

#include "Artifice/Core/Core.h"

// Defines:
#define AR_MAX_DESCRIPTOR_SETS 4
#define AR_MAX_PUSH_CONSTANT_RANGES 2
#define AR_MAX_COLOR_ATTACHMENTS 4 // 97% of devices is 8. spec min is 4.

// Types

enum class QueueType
{
    Universal,
    Compute,
    Present
};

// Gpu is device local memory. used for static buffers.  requires staging buffer to upload
// Cpu is memory that is visible to the cpu and coherent.  this means you don't have to invalidate/flush when mapping.
// CpuToGpu is memory that is visible to the cpu
// TODO: GpuToCpu
enum class MemoryAccessType
{
    Gpu,
    Cpu,
    CpuToGpu,
    GpuToCpu
};

enum class TextureType
{
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture3D,
    TextureCube,
};

enum class DescriptorType
{
    StorageBuffer,
    StorageBufferDynamic,
    StorageImage,
    UniformBuffer,
    UniformBufferDynamic,
    CombinedImageSampler,
};

enum class ShaderStage : uint8
{
    Vertex,
    Fragment,
    Compute,

    Count,
    VertexFragment,
};

enum class LoadOptions
{
    Discard,
    Clear,
    Load
};

enum class StoreOptions
{
    Discard,
    Store
};

enum class RenderBindFlags : uint16
{
    None = 0,
    TransferSource = BIT(0),
    TransferDestination = BIT(1),
    UniformBuffer = BIT(2),
    StorageBuffer = BIT(3),
    VertexBuffer = BIT(4),
    IndexBuffer = BIT(5),
    ColorAttachment = BIT(6),
    DepthStencilAttachment = BIT(7),
    ResolveAttachment = BIT(8),
    InputAttachment = BIT(9),
    SampledTexture = BIT(10),
    StorageTexture = BIT(11),
    Present = BIT(12),
};
DEFINE_BITMASK_OPERATORS(RenderBindFlags, uint16)

enum class PipelineStageFlags : uint16
{
    None = 0,
    Transfer = BIT(0),
    VertexInput = BIT(1),
    Vertex = BIT(2),
    Fragment = BIT(3),
    Compute = BIT(4),
    Color = BIT(5),
    DepthStencil = BIT(6),
    All = BIT(7),
};
DEFINE_BITMASK_OPERATORS(PipelineStageFlags, uint16)

struct ResourceState
{
    RenderBindFlags Bind;
    PipelineStageFlags Stage;

    bool operator==(const ResourceState& other) const
    {
        return Bind == other.Bind && Stage == other.Stage;
    }

    bool operator!=(const ResourceState& other) const
    {
        return !(*this == other);
    }

    static ResourceState None()
    {
        return {RenderBindFlags::None, PipelineStageFlags::None};
    }
    static ResourceState TransferSource()
    {
        return {RenderBindFlags::TransferSource, PipelineStageFlags::Transfer};
    }
    static ResourceState TransferDestination()
    {
        return {RenderBindFlags::TransferDestination, PipelineStageFlags::Transfer};
    }
    static ResourceState Color()
    {
        return {RenderBindFlags::ColorAttachment, PipelineStageFlags::Color};
    }
    static ResourceState Depth()
    {
        return {RenderBindFlags::DepthStencilAttachment, PipelineStageFlags::DepthStencil};
    }
    static ResourceState Resolve()
    {
        return {RenderBindFlags::ResolveAttachment, PipelineStageFlags::Color};
    }
    static ResourceState SampledFragment()
    {
        return {RenderBindFlags::SampledTexture, PipelineStageFlags::Fragment};
    }
    static ResourceState Present()
    {
        return {RenderBindFlags::Present, PipelineStageFlags::None};
    }
    static ResourceState StorageTextureCompute()
    {
        return {RenderBindFlags::StorageTexture, PipelineStageFlags::Compute};
    }
    static ResourceState SampledCompute()
    {
        return {RenderBindFlags::SampledTexture, PipelineStageFlags::Compute};
    }
    static ResourceState VertexBuffer()
    {
        return {RenderBindFlags::VertexBuffer, PipelineStageFlags::VertexInput};
    }
    static ResourceState IndexBuffer()
    {
        return {RenderBindFlags::IndexBuffer, PipelineStageFlags::VertexInput};
    }
};

enum class Format
{
    None,

    RGBA8Unorm,
    BGRA8Unorm,

    R32Float,
    RG32Float,
    RGB32Float,
    RGBA32Float,
    
    RGBA32Int,
    
};
