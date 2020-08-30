#pragma once

#include <vector>
#include <algorithm>
#include <set>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include "Artifice/Core/Core.h"

#include "RenderHandle.h"
#include "Types.h"



// Resources

struct SwapchainInfo
{
    uint32 Width;
    uint32 Height;
    void* Window;
};

struct BufferInfo
{
    RenderBindFlags BindFlags;
    MemoryAccessType Access;
    uint32 Size;

    bool operator==(const BufferInfo& other) const;
    bool operator!=(const BufferInfo& other) const;
    bool operator<(const BufferInfo& other) const;
};

struct TextureInfo
{
    TextureType Type;
    RenderBindFlags BindFlags = RenderBindFlags::None;
    uint32 Width;
    uint32 Height;
    uint32 Depth = 1;
    uint32 Layers = 1;
    bool MipsEnabled = false;
    VkFormat Format;
    VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;

    bool operator==(const TextureInfo& other) const;
    bool operator!=(const TextureInfo& other) const;
    bool operator<(const TextureInfo& other) const;
};

struct SamplerInfo
{
    VkFilter Filter;
    VkSamplerAddressMode AddressMode;
    VkSamplerMipmapMode MipMode;
    uint32 Mips;

    bool operator==(const SamplerInfo& other) const;
    bool operator!=(const SamplerInfo& other) const;
    bool operator<(const SamplerInfo& other) const;
};


struct VertexLayout
{
    struct Binding
    {
        std::vector<Format> Attributes;
        
        Binding() = default;
        Binding(std::initializer_list<Format> attributes);

        bool operator==(const Binding& other) const;
        bool operator!=(const Binding& other) const;
        bool operator<(const Binding& other) const;
    };

    std::vector<Binding> Bindings;

    VertexLayout() = default;
    VertexLayout(std::initializer_list<Binding> bindings);

    bool operator==(const VertexLayout& other) const;
    bool operator!=(const VertexLayout& other) const;
    bool operator<(const VertexLayout& other) const;

    static uint32 GetFormatLocationSize(Format format);
    static uint32 GetFormatSize(Format format);
};

struct ShaderModuleInfo
{
    std::vector<char> Code;
    ShaderStage Stage;
};

struct DescriptorSetLayout
{
    struct Binding
    {
        uint32 Binding;
        DescriptorType Type;
        uint32 Count;
        ShaderStage Stage;
        bool operator==(const DescriptorSetLayout::Binding& other) const;
        bool operator!=(const DescriptorSetLayout::Binding& other) const;
        bool operator<(const DescriptorSetLayout::Binding& other) const;
    };

    std::vector<Binding> Bindings;

    DescriptorSetLayout() = default;
    DescriptorSetLayout(std::initializer_list<DescriptorSetLayout::Binding> bindings);
    void Add(uint32 binding, DescriptorType type, uint32 count, ShaderStage stage);

    bool operator==(const DescriptorSetLayout& other) const;
    bool operator!=(const DescriptorSetLayout& other) const;
    bool operator<(const DescriptorSetLayout& other) const;
};

struct PushConstantRange
{
    ShaderStage Stage;
    uint32 Size;
    uint32 Offset;
};

struct PipelineLayoutInfo
{
    DescriptorSetLayout DescriptorSetLayouts[AR_MAX_DESCRIPTOR_SETS];
    uint32 DescriptorSetLayoutCount;

    PushConstantRange PushConstantRanges[AR_MAX_PUSH_CONSTANT_RANGES];
    uint32 PushConstantRangeCount;
};


// In vulkan terms, this is a compatible render pass
struct RenderPassLayout
{
    struct Attachment
    {
        VkFormat Format;
        VkSampleCountFlagBits Samples;

        bool operator==(const Attachment& other) const;
        bool operator!=(const Attachment& other) const;
        bool operator<(const Attachment& other) const;
    };


    Attachment Colors[AR_MAX_COLOR_ATTACHMENTS];
    Attachment Depth[1];

    uint32 ColorCount = 0;
    uint32 ResolveCount = 0;
    uint32 DepthCount = 0;

    RenderPassLayout() = default;
    RenderPassLayout(uint32 color_count, uint32 resolve_count, uint32 depth_count) : ColorCount(color_count), ResolveCount(resolve_count), DepthCount(depth_count) {}

    bool operator==(const RenderPassLayout& other) const;
    bool operator!=(const RenderPassLayout& other) const;
    bool operator<(const RenderPassLayout& other) const;
};

struct RenderPassInfo
{
    struct AttachmentOptions
    {
        LoadOptions LoadOp = LoadOptions::Discard;
        StoreOptions StoreOp = StoreOptions::Discard;

        bool operator==(const AttachmentOptions& other) const;
        bool operator!=(const AttachmentOptions& other) const;
        bool operator<(const AttachmentOptions& other) const;
    };

    RenderPassInfo() = default;
    RenderPassInfo(RenderPassLayout layout) : Layout(layout) {}

    bool operator==(const RenderPassInfo& other) const;
    bool operator!=(const RenderPassInfo& other) const;
    bool operator<(const RenderPassInfo& other) const;

    RenderPassLayout Layout;

    AttachmentOptions ColorOptions[AR_MAX_COLOR_ATTACHMENTS];
    AttachmentOptions ResolveOptions[AR_MAX_COLOR_ATTACHMENTS];
    AttachmentOptions DepthOptions[1];
};

struct GraphicsPipelineInfo
{
    VertexLayout VertexLayout;
    RenderHandle VertexShaderModule;
    RenderHandle FragmentShaderModule;
    RenderHandle PipelineLayout;
    // Primitive type is triangle list
    // dynamic viewport and scissor
    VkPrimitiveTopology PrimitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
    VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
    bool DepthTestEnabled = true;
    bool DepthWriteEnabled = true;
    bool BlendEnabled = false;
    bool AdditiveBlendEnabled = false;

    RenderPassLayout RenderPassLayout;
};

struct ComputePipelineInfo
{
    RenderHandle ComputeShaderModule;
    RenderHandle PipelineLayout;
};

struct FramebufferInfo
{
    RenderPassLayout RenderPassLayout;
    
    int32 Layers[AR_MAX_COLOR_ATTACHMENTS] = {-1};
    RenderHandle ColorTextures[AR_MAX_COLOR_ATTACHMENTS];
    RenderHandle ResolveTextures[AR_MAX_COLOR_ATTACHMENTS];
    RenderHandle DepthTexture[1];

    uint32 Width;
    uint32 Height;

    FramebufferInfo() = default;

    bool operator==(const FramebufferInfo& other) const;
    bool operator!=(const FramebufferInfo& other) const;
    bool operator<(const FramebufferInfo& other) const;
};

struct DescriptorSetUpdate
{
    struct BufferUpdate
    {
        // StorageBuffer, UniformBuffer
        RenderHandle Buffer;
        uint64 Offset;
        uint64 Range;

        bool operator==(const BufferUpdate& other) const
        {
            if (Buffer != other.Buffer) return false;
            if (Offset != other.Offset) return false;
            if (Range != other.Range) return false;

            return true;
        }
        bool operator!=(const BufferUpdate& other) const
        {
            return !(*this == other);
        }
        bool operator<(const BufferUpdate& other) const
        {
            if (Buffer != other.Buffer) return Buffer < other.Buffer;
            if (Offset != other.Offset) return Offset < other.Offset;
            if (Range != other.Range) return Range < other.Range;

            return false;
        }
    };
    struct ImageUpdate
    {
        //StorageImage, CombinedImageSampler
        RenderHandle Texture;
        RenderBindFlags BindFlag;
        RenderHandle Sampler = RenderHandle::Null();
        int32 TextureLevel = -1;

        bool operator==(const ImageUpdate& other) const
        {
            if (Texture != other.Texture)
                return false;
            if (BindFlag != other.BindFlag)
                return false;
            if (Sampler != other.Sampler)
                return false;
            if (TextureLevel != other.TextureLevel)
                return false;

            return true;
        }
        bool operator!=(const ImageUpdate& other) const
        {
            return !(*this == other);
        }
        bool operator<(const ImageUpdate& other) const
        {
            if (Texture != other.Texture)
                return Texture < other.Texture;
            if (BindFlag != other.BindFlag)
                return BindFlag < other.BindFlag;
            if (Sampler != other.Sampler)
                return Sampler < other.Sampler;
            if (TextureLevel != other.TextureLevel)
                return TextureLevel < other.TextureLevel;

            return false;
        }

    };

    struct Descriptor
    {
        uint32 Binding;
        DescriptorType Type;
        uint32 Count;
        uint32 ArrayOffset;

        std::vector<BufferUpdate> BufferInfos;
        std::vector<ImageUpdate> ImageInfos;


        Descriptor(uint32 binding, DescriptorType type, uint32 count, uint32 array_offset, std::vector<BufferUpdate> buffers)
        {
            Binding = binding;
            Type = type;
            Count = count;
            ArrayOffset = array_offset;
            BufferInfos = buffers;
        }
        Descriptor(uint32 binding, DescriptorType type, uint32 count, uint32 array_offset, std::vector<ImageUpdate> images)
        {
            Binding = binding;
            Type = type;
            Count = count;
            ArrayOffset = array_offset;
            ImageInfos = images;
        }

        bool operator==(const Descriptor& other) const
        {
            if (Binding != other.Binding)
                return false;
            if (Type != other.Type)
                return false;
            if (Count != other.Count)
                return false;
            if (ArrayOffset != other.ArrayOffset)
                return false;
            if (BufferInfos.size() != other.BufferInfos.size()) return false;
            if (ImageInfos.size() != other.ImageInfos.size()) return false;

            for (uint32 i = 0; i < BufferInfos.size(); i++)
            {
                if (BufferInfos[i] != other.BufferInfos[i]) return false;
            }
            for (uint32 i = 0; i < ImageInfos.size(); i++)
            {
                if (ImageInfos[i] != other.ImageInfos[i]) return false;
            }

            return true;
        }
        bool operator!=(const Descriptor& other) const
        {
            return !(*this == other);
        }
        bool operator<(const Descriptor& other) const
        {
            if (Binding != other.Binding)
                return Binding < other.Binding;
            if (Type != other.Type)
                return Type < other.Type;
            if (Count != other.Count)
                return Count < other.Count;
            if (ArrayOffset != other.ArrayOffset)
                return ArrayOffset < other.ArrayOffset;
            if (BufferInfos.size() != other.BufferInfos.size())
                return BufferInfos.size() < other.BufferInfos.size();
            if (ImageInfos.size() != other.ImageInfos.size())
                return ImageInfos.size() < other.ImageInfos.size();

            for (uint32 i = 0; i < BufferInfos.size(); i++)
            {
                if (BufferInfos[i] != other.BufferInfos[i])
                    return BufferInfos[i] < other.BufferInfos[i];
            }
            for (uint32 i = 0; i < ImageInfos.size(); i++)
            {
                if (ImageInfos[i] != other.ImageInfos[i])
                    return ImageInfos[i] < other.ImageInfos[i];
            }

            return false;
        }
    };

    std::vector<Descriptor> Descriptors;
    
    void Add(uint32 binding, DescriptorType type, uint32 count, uint32 array_offset, std::vector<BufferUpdate> buffers)
    {
        Descriptors.push_back(Descriptor(binding, type, count, array_offset, buffers));
    }
    void Add(uint32 binding, DescriptorType type, uint32 count, uint32 array_offset, std::vector<ImageUpdate> images)
    {
        Descriptors.push_back(Descriptor(binding, type, count, array_offset, images));
    }
    void AddUniformBuffer(uint32 binding, RenderHandle buffer, uint64 offset, uint64 range)
    {
        Add(binding, DescriptorType::UniformBuffer, 1, 0, {{buffer, offset, range}});
    }
    void AddUniformBufferDynamic(uint32 binding, RenderHandle buffer, uint64 offset, uint64 range)
    {
        Add(binding, DescriptorType::UniformBufferDynamic, 1, 0, {{buffer, offset, range}});
    }
    void AddStorageBufferDynamic(uint32 binding, RenderHandle buffer, uint64 offset, uint64 range)
    {
        Add(binding, DescriptorType::StorageBufferDynamic, 1, 0, {{buffer, offset, range}});
    }
    void AddCombinedImageSampler(uint32 binding, RenderHandle texture, RenderHandle sampler)
    {
        Add(binding, DescriptorType::CombinedImageSampler, 1, 0, {{texture, RenderBindFlags::SampledTexture, sampler}});
    }
    void AddStorageImage(uint32 binding, RenderHandle texture)
    {
        Add(binding, DescriptorType::StorageImage, 1, 0, {{texture, RenderBindFlags::StorageTexture}});
    }
    bool operator==(const DescriptorSetUpdate& other) const
    {
        if (Descriptors.size() != other.Descriptors.size())
            return false;

        return std::is_permutation(Descriptors.begin(), Descriptors.end(), other.Descriptors.begin());
    }
    bool operator!=(const DescriptorSetUpdate& other) const
    {
        return !(*this == other);
    }
    bool operator<(const DescriptorSetUpdate& other) const
    {
        if (Descriptors.size() != other.Descriptors.size())
            return Descriptors.size() < other.Descriptors.size();

        for (uint32 i = 0; i < Descriptors.size(); i++)
        {
            if (Descriptors[i] != other.Descriptors[i])
                return Descriptors[i] < other.Descriptors[i];
        }

        return false;
    }
};

// Vulkan implementation

struct VulkanResourceState
{
    VkAccessFlags Access;
    VkPipelineStageFlags Stage;
    VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

struct Swapchain
{
    VkSwapchainCreateInfoKHR CreateInfo;
    std::vector<RenderHandle> Textures;

    uint32 ImageIndex;

    VkSwapchainKHR SwapchainRaw;
    VkSurfaceKHR Surface;
    VkSemaphore ImageAvailableSemaphore;
    VkSemaphore RenderFinishedSemaphore;
};

struct Buffer
{
    BufferInfo Info;

    VkBuffer BufferRaw;
    VkDeviceMemory Memory;
};

struct Texture
{
    TextureInfo Info;

    VkImage Image;
    VkDeviceMemory Memory;
    VkImageView ImageView;
    std::vector<VkImageView> ImageViewLayers;
    std::vector<VkImageView> ImageViewLevels;

    uint32 Mips;

    bool IsSwapchainTexture = false;
};

struct Sampler
{
    SamplerInfo Info;
    VkSampler SamplerRaw;
};

struct ShaderModule
{
    ShaderModuleInfo Info;
    VkShaderModule ShaderModuleRaw;
};

struct PipelineLayout
{
    PipelineLayoutInfo Info;

    VkDescriptorSetLayout DescriptorSetLayouts[AR_MAX_DESCRIPTOR_SETS];
    VkPipelineLayout PipelineLayoutRaw;
};

struct RenderPass
{
    RenderPassInfo Info;
    VkRenderPass RenderPassRaw;
};

struct GraphicsPipeline
{
    GraphicsPipelineInfo Info;
    VkPipeline PipelineRaw;
};

struct ComputePipeline
{
    ComputePipelineInfo Info;
    VkPipeline PipelineRaw;
};

struct Framebuffer
{
    FramebufferInfo Info;
    VkFramebuffer FramebufferRaw;
};

struct DescriptorSet
{
    DescriptorSetLayout DescriptorSetLayout;
    VkDescriptorSet DescriptorSetRaw;
};
