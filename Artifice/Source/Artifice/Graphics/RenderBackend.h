#pragma once

#include <functional>
#include <unordered_set>
#include <vector>

#include <vulkan/vulkan.h>

#include "Artifice/Core/Core.h"
#include "Artifice/Debug/Instrumentor.h"
#include "Artifice/Utils/FileUtils.h"
#include "Artifice/Utils/Hash.h"

#include "Artifice/Core/DataBuffer.h"
#include "Artifice/Core/Window.h"

#include "Device.h"
#include "Instance.h"

#include "RenderHandle.h"
#include "RenderHandleAllocator.h"

typedef uint32 DeviceID;

#define RENDER_HANDLE_ALLOCATOR_RING_SIZE 8 + AR_FRAME_COUNT;


static uint32 GetBitIndex(uint32 bit)
{
    for (uint32 i = 0; i <= 32; i++)
    {
        if (bit & BIT(i))
        {
            return i;
        }
    }
    
    AR_CORE_FATAL("");
}
static std::vector<uint32> GetBitIndices(uint32 bits)
{
    std::vector<uint32> indices;

    for (uint32 i = 0; i <= 32; i++)
    {
        if (bits & BIT(i))
        {
            indices.push_back(i);
        }
    }
    
    return indices;
}

// RenderBackend is a representation of a particular graphics API backend
class RenderBackend
{
private:
    Instance m_Instance;
    RenderHandleAllocator<AR_HANDLE_RING_SIZE>* m_RenderHandleAllocator;
    std::vector<Device*> m_Devices; // TODO: better device management

public:
    RenderBackend();
    void Init();
    void CleanUp();

    DeviceID CreateDevice();
    // Default device is BIT(0)
    Device* GetDevice(DeviceID id = BIT(0))
    {
        return m_Devices[GetBitIndex(id)];
    }

    void BeginFrame();
    void EndFrame();
    
    // Should happen as late as possible, right before need access to swapchain images
    bool AcquireSwapchains(std::vector<RenderHandle> swapchains, DeviceID device_id = BIT(0));
    bool PresentSwapchains(QueueType queue, std::vector<RenderHandle> swapchains, DeviceID device_id = BIT(0));

public:
    // Vulkan -> Artifice mapping
    static VkAttachmentLoadOp ConvertAttachmentLoadOptions(LoadOptions options);
    static VkAttachmentStoreOp ConvertAttachmentStoreOptions(StoreOptions options);
    static VkFormat ConvertFormat(Format format);
    static VulkanResourceState ConvertResourceState(ResourceState state, bool zero_access_reads = false);
    static VkDescriptorType ConvertDescriptorType(DescriptorType type);
    static VkShaderStageFlags ConvertShaderStage(ShaderStage stage);
    static VkPipelineStageFlags ConvertPipelineStageFlags(PipelineStageFlags flags);

    static VkAccessFlags ConvertRenderBindFlagsToAccess(RenderBindFlags flags, bool zero_reads = false);
    static VkImageLayout ConvertRenderBindFlagToImageLayout(RenderBindFlags flag);
};
