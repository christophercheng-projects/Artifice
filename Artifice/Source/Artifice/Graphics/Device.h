#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Cache.h"
#include "Artifice/Debug/Instrumentor.h"

#include "Artifice/Graphics/CommandBuffer.h"
#include "Artifice/Graphics/CommandPool.h"
#include "Artifice/Graphics/RenderHandle.h"
#include "Artifice/Graphics/RenderHandleAllocator.h"
#include "Artifice/Graphics/Resources.h"
#include "Artifice/Graphics/ScratchBufferAllocator.h"

#include "Artifice/Graphics/DescriptorSetLayoutInternalCache.h"
#include "Artifice/Graphics/DescriptorSetCache.h"

// TODO: Fence and semaphore management

struct PhysicalDevice
{
    VkPhysicalDevice Handle;
    VkPhysicalDeviceProperties Properties;
    VkPhysicalDeviceMemoryProperties MemoryProperties;
    VkPhysicalDeviceFeatures Features;
    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> SurfaceFormats;
    std::vector<VkPresentModeKHR> PresentModes;
    std::vector<VkQueueFamilyProperties> QueueFamilyProperties;
    std::vector<VkExtensionProperties> ExtensionProperties;
};


class FencePool
{
private:
    VkDevice m_Device;
    std::vector<VkFence> m_Fences;
    uint32 m_Index = 0;

public:
    void Init(VkDevice device) { m_Device = device; }
    void CleanUp()
    {
        for (auto& fence : m_Fences)
        {
            vkDestroyFence(m_Device, fence, nullptr);
        }
    }
    VkFence GetFence()
    {
        if (m_Index >= m_Fences.size())
        {
            VkFence fence;

            VkFenceCreateInfo ci = {};
            ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

            VK_CALL(vkCreateFence(m_Device, &ci, nullptr, &fence));

            m_Fences.push_back(fence);
        }

        return m_Fences[m_Index++];
    }

    void WaitForFences()
    {
        AR_PROFILE_FUNCTION();
        
        if (m_Index > 0)
        {
            VK_CALL(vkWaitForFences(m_Device, m_Index, m_Fences.data(), VK_TRUE, UINT64_MAX));
            VK_CALL(vkResetFences(m_Device, m_Index, m_Fences.data()));
            m_Index = 0;
        }
    }
};

class SemaphoreAllocator
{
private:
    VkDevice m_Device;
    std::vector<VkSemaphore> m_FreeSemaphores;
public:
    void Init(VkDevice device)
    {
        m_Device = device;
    }
    VkSemaphore GetSemaphore()
    {
        VkSemaphore result;
        if (m_FreeSemaphores.size())
        {
            result = m_FreeSemaphores.back();
            m_FreeSemaphores.pop_back();
        }
        else
        {
            VkSemaphoreCreateInfo semaphore_ci = {};
            semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VK_CALL(vkCreateSemaphore(m_Device, &semaphore_ci, nullptr, &result));
        }

        return result;
    }
    void ReleaseSemaphore(VkSemaphore semaphore)
    {
        m_FreeSemaphores.push_back(semaphore);
    }
};


struct Frame
{
    VkDevice Device;

    CommandPool UniversalCommandPool;
    CommandPool ComputeCommandPool;
    std::vector<VkCommandBuffer> CommitedUniversalCommandBuffers;
    std::vector<VkCommandBuffer> CommitedComputeCommandBuffers;

    FencePool GeneralFencePool;
    FencePool TransferFencePool;

    ScratchBufferAllocator ScratchVertexAllocator;
    ScratchBufferAllocator ScratchIndexAllocator;
    ScratchBufferAllocator ScratchUniformAllocator;
    ScratchBufferAllocator ScratchStorageAllocator;

    std::vector<VkBuffer> BuffersToDestroy;
    std::vector<VkImage> ImagesToDestroy;
    std::vector<VkImageView> ImageViewsToDestroy;
    std::vector<VkSampler> SamplersToDestroy;
    std::vector<VkPipeline> PipelinesToDestroy;
    std::vector<VkFramebuffer> FramebuffersToDestroy;
    std::vector<VkRenderPass> RenderPassesToDestroy;
    std::vector<VkSemaphore> SemaphoresToDestroy;
    std::vector<VkDeviceMemory> MemoryToFree;

    std::vector<DescriptorSet> DescriptorSetsToRelease;
    // Temporary hack
    DescriptorSetLayoutInternalCache* DescriptorSetLayoutInternalCache;

    void Begin()
    {
        AR_PROFILE_FUNCTION();
        
        // (Blocking) Wait for this frame to finish from last round of submits
        GeneralFencePool.WaitForFences();
        TransferFencePool.WaitForFences();

        // Reset command pools
        UniversalCommandPool.Reset();
        ComputeCommandPool.Reset();

        // Reset scratch buffer allocators
        ScratchVertexAllocator.Reset();
        ScratchIndexAllocator.Reset();
        ScratchUniformAllocator.Reset();
        ScratchStorageAllocator.Reset();

        // Deferred destroy of resources

        for (auto& buffer : BuffersToDestroy)
        {
            vkDestroyBuffer(Device, buffer, nullptr);
        }
        for (auto& image : ImagesToDestroy)
        {
            vkDestroyImage(Device, image, nullptr);
        }
        for (auto& image_view : ImageViewsToDestroy)
        {
            vkDestroyImageView(Device, image_view, nullptr);
        }
        for (auto& sampler : SamplersToDestroy)
        {
            vkDestroySampler(Device, sampler, nullptr);
        }
        for (auto& pipeline : PipelinesToDestroy)
        {
            vkDestroyPipeline(Device, pipeline, nullptr);
        }
        for (auto& render_pass : RenderPassesToDestroy)
        {
            vkDestroyRenderPass(Device, render_pass, nullptr);
        }
        for (auto& framebuffer : FramebuffersToDestroy)
        {
            vkDestroyFramebuffer(Device, framebuffer, nullptr);
        }
        for (auto& semaphore : SemaphoresToDestroy)
        {
            vkDestroySemaphore(Device, semaphore, nullptr);
        }
        for (auto& memory : MemoryToFree)
        {
            vkFreeMemory(Device, memory, nullptr);
        }

        BuffersToDestroy.clear();
        ImagesToDestroy.clear();
        ImageViewsToDestroy.clear();
        SamplersToDestroy.clear();
        PipelinesToDestroy.clear();
        FramebuffersToDestroy.clear();
        RenderPassesToDestroy.clear();
        SemaphoresToDestroy.clear();
        MemoryToFree.clear();

        for (auto& set : DescriptorSetsToRelease)
        {
            DescriptorSetLayoutInternalCache->RequestPointer(set.DescriptorSetLayout)->ReleaseDescriptorSet(set.DescriptorSetRaw);
        }
        DescriptorSetsToRelease.clear();
    }
};

struct Queue
{
    VkQueue Queue;
    std::vector<VkSemaphore> SignalSemaphores;
    std::vector<VkSemaphore> WaitSemaphores;
    std::vector<PipelineStageFlags> WaitStages;
};

class Device
{
private:
    RenderHandleAllocator<AR_HANDLE_RING_SIZE>* m_RenderHandleAllocator;
private:
    VkInstance m_Instance;
    std::vector<const char*> m_DeviceExtensions;
    PhysicalDevice m_PhysicalDevice;
    VkDevice m_Handle;

private:
    Queue m_InternalUniversalQueue;
    Queue m_InternalComputeQueue;
    Queue m_InternalPresentQueue;
private:
    Queue* m_UniversalQueue = nullptr;
    Queue* m_ComputeQueue = nullptr;
    Queue* m_PresentQueue = nullptr;

    int32 m_UniversalFamilyIndex = -1;
    int32 m_ComputeFamilyIndex = -1;
    int32 m_PresentFamilyIndex = -1;

    bool m_UniversalQueuePresentSupported = false;
    bool m_ComputeQueuePresentSupported = false;

    Queue* GetQueue(QueueType queue)
    {
        AR_CORE_ASSERT(IsQueueSupported(queue), "Unsupported queue type");

        switch (queue)
        {
            case QueueType::Universal:
                return m_UniversalQueue;
            case QueueType::Compute:
                return m_ComputeQueue;
            case QueueType::Present:
                return m_PresentQueue;
            default:
                AR_CORE_FATAL("Unsupported queue type");
        }
    }

public:
    int32 GetQueueFamilyIndex(QueueType queue)
    {
        AR_CORE_ASSERT(IsQueueSupported(queue), "Unsupported queue type");

        switch (queue)
        {
            case QueueType::Universal:
                return m_UniversalFamilyIndex;
            case QueueType::Compute:
                return m_ComputeFamilyIndex;
            case QueueType::Present:
                return m_PresentFamilyIndex;
            default:
                AR_CORE_FATAL("Unsupported queue type");
        }
    }

    bool IsDedicatedComputeSupported() const { return m_ComputeQueue != nullptr; }
    bool IsDedicatedPresentSupported() const { return m_PresentQueue != nullptr; }

    bool IsUniversalQueuePresentSupported() const { return m_UniversalQueuePresentSupported; }
    bool IsComputeQueuePresentSupported() const { return m_ComputeQueuePresentSupported; }

    bool IsQueueSupported(QueueType queue) const
    {
        switch (queue)
        {
            case QueueType::Universal:
                return true;
            case QueueType::Compute:
                return IsDedicatedComputeSupported();
            case QueueType::Present:
                return IsDedicatedPresentSupported();
            default:
                AR_CORE_FATAL("Unsupported queue type");
        }
    }

private:
    RenderHandleResourceStorage<RenderHandleType::Swapchain, Swapchain> m_SwapchainStorage;
    RenderHandleResourceStorage<RenderHandleType::Buffer, Buffer> m_BufferStorage;
    RenderHandleResourceStorage<RenderHandleType::Texture, Texture> m_TextureStorage;
    RenderHandleResourceStorage<RenderHandleType::Sampler, Sampler> m_SamplerStorage;
    RenderHandleResourceStorage<RenderHandleType::ShaderModule, ShaderModule> m_ShaderModuleStorage;
    RenderHandleResourceStorage<RenderHandleType::PipelineLayout, PipelineLayout> m_PipelineLayoutStorage;
    RenderHandleResourceStorage<RenderHandleType::RenderPass, RenderPass> m_RenderPassStorage;
    RenderHandleResourceStorage<RenderHandleType::GraphicsPipeline, GraphicsPipeline> m_GraphicsPipelineStorage;
    RenderHandleResourceStorage<RenderHandleType::ComputePipeline, ComputePipeline> m_ComputePipelineStorage;
    RenderHandleResourceStorage<RenderHandleType::Framebuffer, Framebuffer> m_FramebufferStorage;
    RenderHandleResourceStorage<RenderHandleType::DescriptorSet, DescriptorSet> m_DescriptorSetStorage;

    DescriptorSetLayoutInternalCache m_DescriptorSetLayoutInternalCache;

    DescriptorSetCache m_DescriptorSetCache;


    VkSampleCountFlagBits m_MaxMSAASamples;

    bool m_VSync = true;

    Frame m_Frames[AR_FRAME_COUNT];
    uint32 m_FrameIndex = 0;
public:
    VkSampleCountFlagBits GetMaxSamples() const { return m_MaxMSAASamples; }

public:
    Device();
    ~Device();

public:
    void Init(VkInstance instance, RenderHandleAllocator<AR_HANDLE_RING_SIZE>* allocator);
    void CleanUp();

    inline VkDevice GetHandle() const { return m_Handle; }

private:
    void SelectPhysicalDevice();
    void CreateLogicalDevice();

private:
    bool CheckDeviceExtensionSupport(PhysicalDevice& gpu, const std::vector<const char*>& requiredExtensions);

public:
    // Increment frame index
    void BeginFrame()
    {
        m_FrameIndex = (m_FrameIndex + 1) % AR_FRAME_COUNT;
        GetFrame()->Begin();
        m_DescriptorSetCache.Advance();
    }
    void EndFrame()
    {
    }
    Frame* GetFrame() { return &m_Frames[m_FrameIndex]; }
    uint32 GetFrameIndex() const { return m_FrameIndex; }

    void SetVSync(bool vsync) { m_VSync = vsync; }

public: // RESOURCES
    // Swapchains
    RenderHandle CreateSwapchain(const SwapchainInfo& info);
    std::vector<bool> AcquireSwapchains(std::vector<RenderHandle> swapchain_handles);
    std::vector<bool> PresentSwapchains(QueueType queue_type, std::vector<RenderHandle> swapchain_handles);
    void ResizeSwapchain(RenderHandle swapchain_handle, uint32 width, uint32 height);
    void DestroySwapchain(RenderHandle swapchain_handle);
    // Buffers
    RenderHandle CreateBuffer(const BufferInfo& info);
    void* MapBuffer(RenderHandle buffer_handle);
    void UnmapBuffer(RenderHandle buffer_handle);
    void DestroyBuffer(RenderHandle buffer_handle);
    // Textures
    RenderHandle CreateTexture(const TextureInfo& info);
    void DestroyTexture(RenderHandle texture_handle);
    // Helpers -- user uploads happen via command buffers
    void UploadBufferData(VkCommandBuffer cmd, RenderHandle buffer_handle, uint64 size, const void* data);
    void UploadTextureData(VkCommandBuffer cmd, RenderHandle texture_handle, uint64 size, const void* data);
    void GenerateTextureMips(VkCommandBuffer cmd, RenderHandle texture_handle);
    // Samplers
    RenderHandle CreateSampler(const SamplerInfo& info);
    void DestroySampler(RenderHandle sampler_handle);
    // Shader modules
    RenderHandle CreateShaderModule(const ShaderModuleInfo& info);
    void DestroyShaderModule(RenderHandle shader_module_handle);
    // Pipeline layouts
    RenderHandle CreatePipelineLayout(const PipelineLayoutInfo& info);
    void DestroyPipelineLayout(RenderHandle pipeline_layout_handle);
    // RenderPasses
    RenderHandle CreateRenderPass(const RenderPassInfo& info);
    void DestroyRenderPass(RenderHandle render_pass_handle);
    // Pipeline
    RenderHandle CreateGraphicsPipeline(GraphicsPipelineInfo& info);
    void DestroyGraphicsPipeline(RenderHandle graphics_pipeline_handle);
    RenderHandle CreateComputePipeline(ComputePipelineInfo& info);
    void DestroyComputePipeline(RenderHandle compute_pipeline_handle);
    // Framebuffers
    RenderHandle CreateFramebuffer(const FramebufferInfo& info);
    void DestroyFramebuffer(RenderHandle framebuffer_handle);
    // Descriptor set
    RenderHandle CreateDescriptorSet(const DescriptorSetLayout& layout);
    void UpdateDescriptorSet(RenderHandle descriptor_set, const DescriptorSetUpdate& info);
    void DestroyDescriptorSet(RenderHandle descriptor_set_handle);

    DescriptorSetCache* GetDescriptorSetCache() { return &m_DescriptorSetCache; }

    // Scratch resources
    ScratchBuffer RequestVertexScratchBuffer(uint64 size)
    {
        return GetFrame()->ScratchVertexAllocator.Allocate(size);
    }
    ScratchBuffer RequestIndexScratchBuffer(uint64 size)
    {
        return GetFrame()->ScratchIndexAllocator.Allocate(size);
    }
    ScratchBuffer RequestUniformScratchBuffer(uint64 size, uint32 count = 1)
    {
        return GetFrame()->ScratchUniformAllocator.Allocate(size, count);
    }
    ScratchBuffer RequestStorageScratchBuffer(uint64 size, uint32 count = 1)
    {
        return GetFrame()->ScratchStorageAllocator.Allocate(size, count);
    }

public:
    // TODO: semaphore management
    void DestroySemaphore(VkSemaphore semaphore)
    {
        GetFrame()->SemaphoresToDestroy.push_back(semaphore);
    }
public: // Command buffers, queues, submission, and scheduling
    // Semaphore ops

    void AddSignalReleaseSwapchainSemaphore(QueueType queue, RenderHandle swapchain)
    {
        VkSemaphore semaphore = GetSwapchain(swapchain)->RenderFinishedSemaphore;
        AddSignalSemaphore(queue, semaphore);
    };

    void AddSignalSemaphore(QueueType queue, VkSemaphore semaphore)
    {
        AR_CORE_ASSERT(IsQueueSupported(queue), "Unsupported queue type");

        switch (queue)
        {
        case QueueType::Universal:
            m_UniversalQueue->SignalSemaphores.push_back(semaphore);
            return;
        case QueueType::Compute:
            m_ComputeQueue->SignalSemaphores.push_back(semaphore);
            return;
        case QueueType::Present:
            m_PresentQueue->SignalSemaphores.push_back(semaphore);
            return;
        }
    };

    void AddWaitForAcquireSwapchainSemaphore(QueueType queue, RenderHandle swapchain, PipelineStageFlags stages)
    {
        VkSemaphore semaphore = GetSwapchain(swapchain)->ImageAvailableSemaphore;
        AddWaitSemaphore(queue, semaphore, stages);
    }

    void AddWaitSemaphore(QueueType queue, VkSemaphore semaphore, PipelineStageFlags stages)
    {
        AR_CORE_ASSERT(IsQueueSupported(queue), "Unsupported queue type");

        switch (queue)
        {
        case QueueType::Universal:
            m_UniversalQueue->WaitSemaphores.push_back(semaphore);
            m_UniversalQueue->WaitStages.push_back(stages);
            return;
        case QueueType::Compute:
            m_ComputeQueue->WaitSemaphores.push_back(semaphore);
            m_ComputeQueue->WaitStages.push_back(stages);
            return;
        case QueueType::Present:
            m_PresentQueue->WaitSemaphores.push_back(semaphore);
            m_PresentQueue->WaitStages.push_back(stages);
            return;
        }
    };
    // TODO?: support multiple VkSubmitInfo's per vkQueueSubmit
    // queue_type: which queue to vkQueueSubmit
    // swapchains: optional array of swapchains to signal their semaphores when work completes
    // semaphores: optional array of semaphores to signal when work completes
    // fence: optional fence to signal when work completes
    void SubmitQueue(QueueType queue_type, std::vector<RenderHandle> swapchains = {}, std::vector<VkSemaphore> semaphores = {}, bool fence = true);

    // TODO: Current command buffer api is really sus.
    CommandBuffer RequestCommandBuffer(QueueType type = QueueType::Universal)
    {
        switch (type)
        {
            case QueueType::Universal:
                return CommandBuffer(this, type, GetFrame()->UniversalCommandPool.GetCommandBuffer());
            case QueueType::Compute:
                return CommandBuffer(this, type, GetFrame()->ComputeCommandPool.GetCommandBuffer());
            default:
                AR_CORE_FATAL("");
        }
    }
    void CommitCommandBuffer(CommandBuffer command_buffer)
    {
        switch (command_buffer.GetType())
        {
            case QueueType::Universal:
                GetFrame()->CommitedUniversalCommandBuffers.push_back(command_buffer.m_CommandBuffer);
                break;
            case QueueType::Compute:
                GetFrame()->CommitedComputeCommandBuffers.push_back(command_buffer.m_CommandBuffer);
                break;
            default:
                AR_CORE_FATAL("");
        }
    }

    void WaitIdle()
    {
        AR_PROFILE_FUNCTION();
        
        VK_CALL(vkDeviceWaitIdle(m_Handle));
    }

    void WaitForFences()
    {
        AR_PROFILE_FUNCTION()
        
        GetFrame()->GeneralFencePool.WaitForFences();
        GetFrame()->TransferFencePool.WaitForFences();

    }

public:
    const BufferInfo& GetBufferInfo(RenderHandle handle)
    {
        return GetBuffer(handle)->Info;
    }
    const TextureInfo& GetTextureInfo(RenderHandle handle)
    {
        return GetTexture(handle)->Info;
    }
    std::pair<uint32, uint32> GetSwapchainDimensions(RenderHandle handle)
    {
        Swapchain* swapchain = GetSwapchain(handle);
        return {swapchain->CreateInfo.imageExtent.width, swapchain->CreateInfo.imageExtent.height};
    }

    RenderHandle GetSwapchainTexture(RenderHandle swapchain)
    {
        Swapchain* swap = GetSwapchain(swapchain);
        return swap->Textures[swap->ImageIndex];
    }

    const TextureInfo& GetSwapchainTextureInfo(RenderHandle handle)
    {
        Swapchain* swap = GetSwapchain(handle);
        return GetTextureInfo(swap->Textures[0]);
    }

    VkFormat GetSwapchainTextureFormat(RenderHandle swapchain)
    {
        Swapchain* swap = GetSwapchain(swapchain);

        return swap->CreateInfo.imageFormat;
    }

public: // VULKAN ONLY
    // Resource getters (raw, API dependent)
    Swapchain* GetSwapchain(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_SwapchainStorage.Get(handle);
    }
    Buffer* GetBuffer(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_BufferStorage.Get(handle);
    }
    Texture* GetTexture(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_TextureStorage.Get(handle);
    }
    Sampler* GetSampler(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_SamplerStorage.Get(handle);
    }
    ShaderModule* GetShaderModule(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_ShaderModuleStorage.Get(handle);
    }
    PipelineLayout* GetPipelineLayout(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_PipelineLayoutStorage.Get(handle);
    }
    RenderPass* GetRenderPass(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_RenderPassStorage.Get(handle);
    }
    GraphicsPipeline* GetGraphicsPipeline(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_GraphicsPipelineStorage.Get(handle);
    }
    ComputePipeline* GetComputePipeline(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_ComputePipelineStorage.Get(handle);
    }
    Framebuffer* GetFramebuffer(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_FramebufferStorage.Get(handle);
    }
    DescriptorSet* GetDescriptorSet(RenderHandle handle)
    {
        AR_CORE_ASSERT(m_RenderHandleAllocator->IsValid(handle), "");

        return m_DescriptorSetStorage.Get(handle);
    }

private:
    // Helpers
    void CreateRawBuffer(VkBuffer* buffer, VkDeviceMemory* memory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties);
    void CreateRawImage(VkImage* image, VkDeviceMemory* memory, VkImageType type, uint32 width, uint32 height, uint32 depth, uint32 mips, uint32 layers, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples, VkFlags flags, VkMemoryPropertyFlags memory_properties);
    void CreateRawImageView(VkImageView* view, VkImage image, VkImageViewType type, VkFormat format, uint32 base_mip, uint32 mips, uint32 base_layer, uint32 layers);
    void CreateRawSampler(VkSampler* sampler, VkFilter filter, VkSamplerAddressMode mode, VkSamplerMipmapMode mip_mode, uint32 mips);
    void CreateRawRenderPass(VkRenderPass* render_pass, uint32 attachment_count, VkAttachmentDescription* attachments, uint32 color_count, VkAttachmentReference* color_refs, VkAttachmentReference* resolve_refs, VkAttachmentReference* depth_ref);

private:
    // * implemented in DeviceUtil.cpp *
    // Utility functions for convenience

    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D ChooseSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32 width, uint32 height);

    uint32 FindMemoryType(unsigned int typeFilter, VkMemoryPropertyFlags properties);
    bool IsImageFormatSupported(VkFormat format, VkFormatFeatureFlags features, VkImageTiling tiling);
public:
    VkFormat GetDefaultDepthFormat();
    uint64 GetAlignedUniformBufferSize(uint64 size)
    {
        uint32 remainder = size % 256;
        if (remainder == 0)
        {
            return size;
        }
        else
        {
            return size + 256 - remainder;
        }
    }
    uint64 GetAlignedStorageBufferSize(uint64 size)
    {
        uint32 remainder = size % 256;
        if (remainder == 0)
        {
            return size;
        }
        else
        {
            return size + 256 - remainder;
        }
    }

public:
    // helpers TODO: reorganize
    static VkImageUsageFlags GetAttachmentUsageFromFormat(VkFormat format);
    static VkPipelineStageFlags GetStagesFromImageUsage(VkImageUsageFlags usage);
    static VkAccessFlags GetAccessFromImageUsage(VkImageUsageFlags usage);
    static VkAccessFlags GetAccessFromImageLayout(VkImageLayout layout);
    static VkImageAspectFlags GetAspectFlagsFromFormat(VkFormat format);

};
