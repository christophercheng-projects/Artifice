#include "Device.h"

#include <set>

#include "RenderBackend.h"

#include "Artifice/Core/Application.h"
#include "Artifice/Core/ScratchAllocator.h"
#include "Artifice/Core/Log.h"

#include "Artifice/math/math_functions.h"

Device::Device()
{
}

Device::~Device()
{
}

void Device::Init(VkInstance instance, RenderHandleAllocator<AR_HANDLE_RING_SIZE>* allocator)
{
    AR_PROFILE_FUNCTION();

    m_Instance = instance;
    m_RenderHandleAllocator = allocator;
    m_DeviceExtensions.reserve(1);
    m_DeviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    SelectPhysicalDevice();
    CreateLogicalDevice();

    AR_CORE_INFO("Device created with queues: ")
    AR_CORE_INFO("\tUniversal Queue is supported with family index: %d", m_UniversalFamilyIndex);
    if (IsUniversalQueuePresentSupported())
    {
        AR_CORE_INFO("\tUniversal Queue supports present");
    }
    else
    {
        AR_CORE_INFO("\tUniversal Queue does NOT support present");
    }

    if (IsDedicatedComputeSupported())
    {
        AR_CORE_INFO("\tDedicated Compute Queue is supported with family index: %d", m_ComputeFamilyIndex);

        if (IsUniversalQueuePresentSupported())
        {
            AR_CORE_INFO("\tDedicated Compute Queue supports present");
        }
        else
        {
            AR_CORE_INFO("\tDedicated Compute Queue does NOT support present");
        }
    }
    else
    {
        AR_CORE_INFO("\tDedicated Compute Queue is NOT supported");
    }

    if (IsDedicatedPresentSupported())
    {
        AR_CORE_INFO("\tDedicated Present Queue is supported with family index: %d", m_PresentFamilyIndex);
    }
    else
    {
        AR_CORE_INFO("\tDedicated Present Queue is NOT supported");
    }

    m_DescriptorSetLayoutInternalCache.Init(m_Handle);
    m_DescriptorSetCache.Init(this);

    for (uint32 i = 0; i < AR_FRAME_COUNT; i++)
    {
        Frame& frame = m_Frames[i];

        frame.Device = m_Handle;
        // TODO: should store actual device
        frame.DescriptorSetLayoutInternalCache = &m_DescriptorSetLayoutInternalCache;
        frame.GeneralFencePool.Init(m_Handle);
        frame.UniversalCommandPool.Init(m_Handle, m_UniversalFamilyIndex);
        frame.ComputeCommandPool.Init(m_Handle, m_ComputeFamilyIndex);

        frame.ScratchVertexAllocator = ScratchBufferAllocator(this, 1024 * 4, RenderBindFlags::VertexBuffer);
        frame.ScratchIndexAllocator = ScratchBufferAllocator(this, 1024 * 4, RenderBindFlags::IndexBuffer);
        frame.ScratchUniformAllocator = ScratchBufferAllocator(this, 1024 * 4, RenderBindFlags::UniformBuffer);
        frame.ScratchStorageAllocator = ScratchBufferAllocator(this, 1024 * 4, RenderBindFlags::StorageBuffer);
    }

}

void Device::CleanUp()
{
    AR_PROFILE_FUNCTION();

    WaitIdle();

    for (uint32 i = 0; i < AR_FRAME_COUNT; i++)
    {
        Frame& frame = m_Frames[i];

        frame.ScratchVertexAllocator.CleanUp();
        frame.ScratchIndexAllocator.CleanUp();
        frame.ScratchUniformAllocator.CleanUp();
        frame.ScratchStorageAllocator.CleanUp();
    }
    for (uint32 i = 0; i < AR_FRAME_COUNT; i++)
    {
        Frame& frame = m_Frames[i];

        frame.Begin();

        frame.UniversalCommandPool.CleanUp();
        frame.ComputeCommandPool.CleanUp();
        frame.GeneralFencePool.CleanUp();
        frame.TransferFencePool.CleanUp();
    }

    m_DescriptorSetCache.Reset();
    m_DescriptorSetLayoutInternalCache.Reset();

    vkDestroyDevice(m_Handle, nullptr);
}

void Device::SubmitQueue(QueueType queue_type, std::vector<RenderHandle> swapchains, std::vector<VkSemaphore> semaphores, bool fence)
{
    AR_PROFILE_FUNCTION();

    AR_CORE_ASSERT(IsQueueSupported(queue_type), "Unsupported queue type");

    // assume graphics for now
    Queue* queue = GetQueue(queue_type);

    for (auto& swapchain : swapchains)
    {
        Swapchain* swap = GetSwapchain(swapchain);
        semaphores.push_back(swap->RenderFinishedSemaphore);
    }
    for (auto sem : queue->SignalSemaphores)
    {
        semaphores.push_back(sem);
    }
    std::vector<VkPipelineStageFlags> wait_stages;
    for (auto& stage : queue->WaitStages)
    {
        VkPipelineStageFlags stage_raw = RenderBackend::ConvertPipelineStageFlags(stage);
        if (stage_raw == 0)
            stage_raw = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        wait_stages.push_back(stage_raw);
    }

    std::vector<VkCommandBuffer>* cbs;
    switch (queue_type)
    {
    case QueueType::Universal:
        cbs = &GetFrame()->CommitedUniversalCommandBuffers;
        break;
    case QueueType::Compute:
        cbs = &GetFrame()->CommitedComputeCommandBuffers;
        break;
    default:
        AR_CORE_FATAL("");
    }

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = queue->WaitSemaphores.size();
    submit_info.pWaitSemaphores = queue->WaitSemaphores.data();
    submit_info.pWaitDstStageMask = wait_stages.data();
    submit_info.commandBufferCount = cbs->size();
    submit_info.pCommandBuffers = cbs->data();
    submit_info.signalSemaphoreCount = semaphores.size();
    submit_info.pSignalSemaphores = semaphores.data();

    VkFence fence_raw = VK_NULL_HANDLE;
    if (fence)
    {
        fence_raw = GetFrame()->GeneralFencePool.GetFence();
    }

    VK_CALL(vkQueueSubmit(queue->Queue, 1, &submit_info, fence_raw));

    cbs->clear();

    queue->SignalSemaphores.clear();
    queue->WaitSemaphores.clear();
    queue->WaitStages.clear();
}

// Swapchains

RenderHandle Device::CreateSwapchain(const SwapchainInfo& info)
{
    AR_PROFILE_FUNCTION();

    VkSurfaceKHR surface;
    VK_CALL(glfwCreateWindowSurface(m_Instance, (GLFWwindow*)info.Window, nullptr, &surface));

    // TODO: clean up the whole mess with different surfaces/present capabilities/etc.
    if (IsUniversalQueuePresentSupported())
    {
        VkBool32 universal_present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice.Handle, m_UniversalFamilyIndex, surface, &universal_present);
        if (!universal_present)
            AR_CORE_FATAL("Attempt to create swapchain failed. Present from universal queue not supported");
    }
    if (IsComputeQueuePresentSupported())
    {
        VkBool32 compute_present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice.Handle, m_ComputeFamilyIndex, surface, &compute_present);
        if (!compute_present)
            AR_CORE_FATAL("Attempt to create swapchain failed. Present from compute queue not supported");
    }
    if (IsDedicatedPresentSupported())
    {
        VkBool32 separate_present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice.Handle, m_PresentFamilyIndex, surface, &separate_present);
        if (!separate_present)
            AR_CORE_FATAL("Attempt to create swapchain failed. Present from separate present queue not supported");
    }

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice.Handle, surface, &surface_capabilities));

    uint32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice.Handle, surface, &format_count, nullptr);
    AR_CORE_ASSERT(format_count > 0, "No surface formats found");

    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice.Handle, surface, &format_count, surface_formats.data());
    AR_CORE_ASSERT(format_count > 0, "No surface formats found");

    uint32 present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice.Handle, surface, &present_modes_count, nullptr);
    AR_CORE_ASSERT(present_modes_count > 0, "No present modes found");

    std::vector<VkPresentModeKHR> present_modes(present_modes_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice.Handle, surface, &present_modes_count, present_modes.data());
    AR_CORE_ASSERT(present_modes_count > 0, "No present modes found");

    VkSurfaceFormatKHR surface_format = ChooseSurfaceFormat(surface_formats);
    VkPresentModeKHR present_mode = ChoosePresentMode(present_modes);
    VkExtent2D surface_extent = ChooseSurfaceExtent(surface_capabilities, info.Width, info.Height);
    uint32 image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
    {
        image_count = surface_capabilities.maxImageCount;
    }

    VkSwapchainKHR swapchain;

    VkSwapchainCreateInfoKHR ci = {};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = surface;
    ci.minImageCount = image_count;
    ci.imageFormat = surface_format.format;
    ci.imageColorSpace = surface_format.colorSpace;
    ci.imageExtent = surface_extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = present_mode;
    ci.clipped = VK_TRUE;

    VK_CALL(vkCreateSwapchainKHR(m_Handle, &ci, nullptr, &swapchain));

    // Get swapchain images
    vkGetSwapchainImagesKHR(m_Handle, swapchain, &image_count, nullptr);
    AR_CORE_ASSERT(image_count > 0, "No images found");

    std::vector<VkImage> images(image_count);
    vkGetSwapchainImagesKHR(m_Handle, swapchain, &image_count, images.data());
    AR_CORE_ASSERT(image_count > 0, "No images found");
    AR_CORE_INFO("Swapchain image count: %d", image_count);

    std::vector<RenderHandle> texture_handles(image_count);
    std::vector<Texture> textures(image_count);
    TextureInfo texture_info;
    texture_info.Type = TextureType::Texture2D;
    texture_info.Width = surface_extent.width;
    texture_info.Height = surface_extent.height;
    texture_info.BindFlags = RenderBindFlags::ColorAttachment | RenderBindFlags::TransferDestination;
    texture_info.Format = surface_format.format;
    for (uint32 i = 0; i < image_count; i++)
    {
        VkImageView view;
        CreateRawImageView(&view, images[i], VK_IMAGE_VIEW_TYPE_2D, texture_info.Format, 0, 1, 0, 1);
        textures[i].Image = images[i];
        textures[i].Info = texture_info;
        textures[i].ImageView = view;
        textures[i].IsSwapchainTexture = true;
        textures[i].Mips = 1;

        texture_handles[i] = m_RenderHandleAllocator->Allocate(RenderHandleType::Texture);
        m_TextureStorage.Add(texture_handles[i], textures[i]);
    }

    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkSemaphoreCreateInfo semaphore_ci = {};
    semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CALL(vkCreateSemaphore(m_Handle, &semaphore_ci, nullptr, &image_available_semaphore));
    VK_CALL(vkCreateSemaphore(m_Handle, &semaphore_ci, nullptr, &render_finished_semaphore));

    Swapchain resource = {};
    resource.CreateInfo = ci;
    resource.Textures = texture_handles;
    resource.SwapchainRaw = swapchain;
    resource.Surface = surface;
    resource.ImageAvailableSemaphore = image_available_semaphore;
    resource.RenderFinishedSemaphore = render_finished_semaphore;

    RenderHandle swapchain_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::Swapchain);
    m_SwapchainStorage.Add(swapchain_handle, resource);

    return swapchain_handle;
}

std::vector<bool> Device::AcquireSwapchains(std::vector<RenderHandle> swapchain_handles)
{
    AR_PROFILE_FUNCTION();

    uint32 swapchain_count = swapchain_handles.size();

    std::vector<bool> results(swapchain_count);

    for (uint32 i = 0; i < swapchain_count; i++)
    {
        Swapchain* swapchain = GetSwapchain(swapchain_handles[i]);

        VkResult result = vkAcquireNextImageKHR(m_Handle, swapchain->SwapchainRaw, UINT64_MAX, swapchain->ImageAvailableSemaphore, VK_NULL_HANDLE, &swapchain->ImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            results[i] = false;
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
            results[i] = true;
        }
        else if (result != VK_SUCCESS)
        {
            AR_FATAL("Failed to acquire swapchain image");
        }
        else
        {
            results[i] = true;
        }
    }

    return results;
}

std::vector<bool> Device::PresentSwapchains(QueueType queue_type, std::vector<RenderHandle> swapchain_handles)
{
    AR_PROFILE_FUNCTION();

    ScopedAllocator alloc;

    uint32 swapchain_count = swapchain_handles.size();

    VkSwapchainKHR* swapchains = alloc.Allocate<VkSwapchainKHR>(swapchain_count);
    VkImage* images = alloc.Allocate<VkImage>(swapchain_count);
    uint32* image_indices = alloc.Allocate<uint32>(swapchain_count);
    VkSemaphore* wait_semaphores = alloc.Allocate<VkSemaphore>(swapchain_count);

    for (uint32 i = 0; i < swapchain_count; i++)
    {

        Swapchain* swapchain = GetSwapchain(swapchain_handles[i]);
        swapchains[i] = swapchain->SwapchainRaw;
        Texture* texture = GetTexture(swapchain->Textures[swapchain->ImageIndex]);
        images[i] = texture->Image;
        image_indices[i] = swapchain->ImageIndex;
        wait_semaphores[i] = swapchain->RenderFinishedSemaphore;
    }

    // Present

    VkResult* results_raw = alloc.Allocate<VkResult>(swapchain_count);
    // There is a validation bug if results are not preinitialized to VK_SUCCESS
    for (uint32 i = 0; i < swapchain_count; i++)
    {
        results_raw[i] = VK_SUCCESS;
    }

    Queue* queue = GetQueue(queue_type);

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = swapchain_count;
    present_info.pWaitSemaphores = wait_semaphores;
    present_info.swapchainCount = swapchain_count;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = image_indices;
    present_info.pResults = results_raw;

    std::vector<bool> results(swapchain_handles.size(), true);

    VkResult result = vkQueuePresentKHR(queue->Queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        for (uint32 i = 0; i < swapchain_count; i++)
        {
            VkResult raw = results_raw[i];
            if (raw == VK_ERROR_OUT_OF_DATE_KHR || raw == VK_SUBOPTIMAL_KHR)
            {
                results[i] = false;
            }
        }
    }
    else if (result != VK_SUCCESS)
    {
        AR_FATAL("Failed to present swapchain images(s)");
    }

    return results;
}

void Device::ResizeSwapchain(RenderHandle swapchain_handle, uint32 width, uint32 height)
{
    AR_PROFILE_FUNCTION();

    Swapchain* swapchain = GetSwapchain(swapchain_handle);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice.Handle, swapchain->Surface, &surface_capabilities));

    uint32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice.Handle, swapchain->Surface, &format_count, nullptr);
    AR_CORE_ASSERT(format_count > 0, "No surface formats found");

    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice.Handle, swapchain->Surface, &format_count, surface_formats.data());
    AR_CORE_ASSERT(format_count > 0, "No surface formats found");

    uint32 present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice.Handle, swapchain->Surface, &present_modes_count, nullptr);
    AR_CORE_ASSERT(present_modes_count > 0, "No present modes found");

    std::vector<VkPresentModeKHR> present_modes(present_modes_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice.Handle, swapchain->Surface, &present_modes_count, present_modes.data());
    AR_CORE_ASSERT(present_modes_count > 0, "No present modes found");

    VkSurfaceFormatKHR surface_format = ChooseSurfaceFormat(surface_formats);
    VkPresentModeKHR present_mode = ChoosePresentMode(present_modes);
    VkExtent2D surface_extent = ChooseSurfaceExtent(surface_capabilities, width, height);
    uint32 image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
    {
        image_count = surface_capabilities.maxImageCount;
    }

    VkSwapchainKHR old_swapchain = swapchain->SwapchainRaw;

    swapchain->CreateInfo.minImageCount = image_count;
    swapchain->CreateInfo.imageFormat = surface_format.format;
    swapchain->CreateInfo.imageColorSpace = surface_format.colorSpace;
    swapchain->CreateInfo.imageExtent = surface_extent;
    swapchain->CreateInfo.presentMode = present_mode;
    swapchain->CreateInfo.oldSwapchain = old_swapchain;

    VK_CALL(vkCreateSwapchainKHR(m_Handle, &swapchain->CreateInfo, nullptr, &swapchain->SwapchainRaw));

    for (auto& texture : swapchain->Textures)
    {
        DestroyTexture(texture);
    }

    vkDestroySwapchainKHR(m_Handle, old_swapchain, nullptr);

    // Get swapchain images
    vkGetSwapchainImagesKHR(m_Handle, swapchain->SwapchainRaw, &image_count, nullptr);
    AR_CORE_ASSERT(image_count > 0, "No images found");

    std::vector<VkImage> images(image_count);
    vkGetSwapchainImagesKHR(m_Handle, swapchain->SwapchainRaw, &image_count, images.data());
    AR_CORE_ASSERT(image_count > 0, "No images found");
    AR_CORE_INFO("Swapchain image count: %d", image_count);

    std::vector<RenderHandle> texture_handles(image_count);
    std::vector<Texture> textures(image_count);
    TextureInfo texture_info;
    texture_info.Type = TextureType::Texture2D;
    texture_info.Width = surface_extent.width;
    texture_info.Height = surface_extent.height;
    texture_info.BindFlags = RenderBindFlags::ColorAttachment | RenderBindFlags::TransferDestination;
    texture_info.Format = surface_format.format;
    for (uint32 i = 0; i < image_count; i++)
    {
        VkImageView view;
        CreateRawImageView(&view, images[i], VK_IMAGE_VIEW_TYPE_2D, texture_info.Format, 0, 1, 0, 1);
        textures[i].Image = images[i];
        textures[i].Info = texture_info;
        textures[i].ImageView = view;
        textures[i].IsSwapchainTexture = true;
        textures[i].Mips = 1;

        texture_handles[i] = m_RenderHandleAllocator->Allocate(RenderHandleType::Texture);
        m_TextureStorage.Add(texture_handles[i], textures[i]);
    }

    swapchain->Textures = texture_handles;

    AR_INFO("Resized swapchain to (%i, %i)", surface_extent.width, surface_extent.height);
}

void Device::DestroySwapchain(RenderHandle swapchain_handle)
{
    AR_PROFILE_FUNCTION();

    Swapchain* swapchain = GetSwapchain(swapchain_handle);

    for (auto& texture : swapchain->Textures)
    {
        DestroyTexture(texture);
    }

    vkDestroySwapchainKHR(m_Handle, swapchain->SwapchainRaw, nullptr);

    vkDestroySurfaceKHR(m_Instance, swapchain->Surface, nullptr);

    vkDestroySemaphore(m_Handle, swapchain->ImageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_Handle, swapchain->RenderFinishedSemaphore, nullptr);

    m_RenderHandleAllocator->Release(swapchain_handle);
    m_SwapchainStorage.Remove(swapchain_handle);
}

// Buffers

RenderHandle Device::CreateBuffer(const BufferInfo& info)
{
    AR_PROFILE_FUNCTION();

    VkBufferUsageFlags usage_flags = 0;
    if (Contains(info.BindFlags, RenderBindFlags::TransferSource) || Contains(info.BindFlags, RenderBindFlags::TransferDestination))
    {
        usage_flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::TransferDestination))
    {
        usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::UniformBuffer))
    {
        usage_flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::StorageBuffer))
    {
        usage_flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::VertexBuffer))
    {
        usage_flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::IndexBuffer))
    {
        usage_flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    VkMemoryPropertyFlags memory_flags;
    switch (info.Access)
    {
    case MemoryAccessType::Gpu:
        memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    case MemoryAccessType::Cpu:
        memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    case MemoryAccessType::CpuToGpu:
        memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        break;
    default:
        break;
    }

    VkBuffer buffer;
    VkDeviceMemory memory;
    CreateRawBuffer(&buffer, &memory, info.Size, usage_flags, memory_flags);

    Buffer resource;
    resource.Info = info;
    resource.BufferRaw = buffer;
    resource.Memory = memory;

    RenderHandle render_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::Buffer);
    m_BufferStorage.Add(render_handle, resource);

    return render_handle;
}

void* Device::MapBuffer(RenderHandle buffer_handle)
{
    AR_PROFILE_FUNCTION();

    void* data;
    Buffer* buffer = GetBuffer(buffer_handle);
    vkMapMemory(m_Handle, buffer->Memory, 0, buffer->Info.Size, 0, &data);

    return data;
}
void Device::UnmapBuffer(RenderHandle buffer_handle)
{
    AR_PROFILE_FUNCTION();

    Buffer* buffer = GetBuffer(buffer_handle);
    vkUnmapMemory(m_Handle, buffer->Memory);
}

void Device::UploadBufferData(VkCommandBuffer cmd, RenderHandle buffer_handle, uint64 size, const void* data)
{
    AR_PROFILE_FUNCTION();

    Buffer* buffer = GetBuffer(buffer_handle);

    AR_ASSERT(buffer->Info.Access == MemoryAccessType::Gpu, "Tried uploading to non-GPU-only buffer");

    VkBuffer buffer_raw = buffer->BufferRaw;

    // Create staging buffer

    BufferInfo staging_info;
    staging_info.BindFlags = RenderBindFlags::TransferSource;
    staging_info.Access = MemoryAccessType::Cpu;
    staging_info.Size = size;

    RenderHandle staging_handle = CreateBuffer(staging_info);
    VkBuffer staging_buffer = GetBuffer(staging_handle)->BufferRaw;

    void* ptr = MapBuffer(staging_handle);
    memcpy(ptr, data, size);
    UnmapBuffer(staging_handle);

    // Copy from staging buffer to target buffer

    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(cmd, staging_buffer, buffer_raw, 1, &copy_region);

    // Deferred destroy automatically ensures that buffer is safely deleted
    DestroyBuffer(staging_handle);
}

void Device::DestroyBuffer(RenderHandle buffer_handle)
{
    AR_PROFILE_FUNCTION();

    Buffer* buffer = GetBuffer(buffer_handle);
    GetFrame()->BuffersToDestroy.push_back(buffer->BufferRaw);
    GetFrame()->MemoryToFree.push_back(buffer->Memory);

    m_RenderHandleAllocator->Release(buffer_handle);
    m_BufferStorage.Remove(buffer_handle);
}

// Textures

RenderHandle Device::CreateTexture(const TextureInfo& info)
{
    AR_PROFILE_FUNCTION();

    VkImageType image_type;
    VkImageViewType view_type;
    VkFlags flags = 0;
    switch (info.Type)
    {
    case TextureType::Texture1D:
        image_type = VK_IMAGE_TYPE_1D;
        view_type = VK_IMAGE_VIEW_TYPE_1D;
        break;
    case TextureType::Texture1DArray:
        image_type = VK_IMAGE_TYPE_1D;
        view_type = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        break;
    case TextureType::Texture2D:
        image_type = VK_IMAGE_TYPE_2D;
        view_type = VK_IMAGE_VIEW_TYPE_2D;
        break;
    case TextureType::Texture2DArray:
        image_type = VK_IMAGE_TYPE_2D;
        view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        break;
    case TextureType::TextureCube:
        image_type = VK_IMAGE_TYPE_2D;
        view_type = VK_IMAGE_VIEW_TYPE_CUBE;
        flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        break;
    case TextureType::Texture3D:
        image_type = VK_IMAGE_TYPE_3D;
        view_type = VK_IMAGE_VIEW_TYPE_3D;
        break;
    default:
        break;
    }

    VkImageUsageFlags usage_flags = 0;
    if (Contains(info.BindFlags, RenderBindFlags::TransferSource) || Contains(info.BindFlags, RenderBindFlags::TransferDestination))
    {
        usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::TransferDestination))
    {
        usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::ColorAttachment))
    {
        usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::ResolveAttachment))
    {
        usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::DepthStencilAttachment))
    {
        usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::SampledTexture))
    {
        usage_flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (Contains(info.BindFlags, RenderBindFlags::StorageTexture))
    {
        usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    VkFormat format = info.Format;
    VkSampleCountFlagBits samples = info.Samples;

    uint32 mips = 1;
    if (info.MipsEnabled)
    {
        // TODO: move to helper function
        // TODO: factor in depth for mips
        mips = (uint32)std::floor(std::log2(std::max(info.Width, info.Height))) + 1;
    }

    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    std::vector<VkImageView> view_layers;
    std::vector<VkImageView> view_levels;

    CreateRawImage(&image, &memory, image_type, info.Width, info.Height, info.Depth, mips, info.Layers, format, usage_flags, samples, flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CreateRawImageView(&view, image, view_type, format, 0, mips, 0, info.Layers);

    // TODO, individual image views of individual layers.
    // cubemap layers
    if (info.Type == TextureType::TextureCube)
    {
        for (uint32 i = 0; i < 6; i++)
        {
            VkImageView layer_view;
            CreateRawImageView(&layer_view, image, VK_IMAGE_VIEW_TYPE_2D, format, 0, mips, i, 1);
            view_layers.push_back(layer_view);
        }
    }
    // miplevels : TODO: toggle, don't always need to access individual levels
    for (uint32 i = 0; i < mips; i++)
    {
        VkImageView level_view;
        CreateRawImageView(&level_view, image, view_type, format, i, 1, 0, info.Layers);
        view_levels.push_back(level_view);
    }

    Texture resource;
    resource.Info = info;
    resource.Image = image;
    resource.Memory = memory;
    resource.ImageView = view;
    resource.ImageViewLayers = view_layers;
    resource.ImageViewLevels = view_levels;
    resource.Mips = mips;

    RenderHandle render_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::Texture);
    m_TextureStorage.Add(render_handle, resource);

    return render_handle;
}

void Device::UploadTextureData(VkCommandBuffer cmd, RenderHandle texture_handle, uint64 size, const void* data)
{
    AR_PROFILE_FUNCTION();

    Texture* texture = GetTexture(texture_handle);

    // Create staging buffer to store texture data

    BufferInfo buffer_info;
    buffer_info.BindFlags = RenderBindFlags::TransferSource;
    buffer_info.Access = MemoryAccessType::Cpu;
    buffer_info.Size = size;

    RenderHandle staging_handle = CreateBuffer(buffer_info);
    void* ptr = MapBuffer(staging_handle);
    memcpy(ptr, data, size);
    UnmapBuffer(staging_handle);

    VkBuffer staging_buffer = GetBuffer(staging_handle)->BufferRaw;

    // Copy buffer to texture

    VkBufferImageCopy copy_region = {};
    copy_region.bufferOffset = 0;
    copy_region.bufferRowLength = 0;
    copy_region.bufferImageHeight = 0;
    copy_region.imageSubresource.aspectMask = GetAspectFlagsFromFormat(texture->Info.Format);
    copy_region.imageSubresource.mipLevel = 0;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount = texture->Info.Layers;
    copy_region.imageOffset = {0, 0, 0};
    copy_region.imageExtent = {texture->Info.Width, texture->Info.Height, texture->Info.Depth};
    vkCmdCopyBufferToImage(cmd, staging_buffer, texture->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    // Deferred destroy ensures staging buffer is safely destroyed
    DestroyBuffer(staging_handle);
}

void Device::GenerateTextureMips(VkCommandBuffer cmd, RenderHandle texture_handle)
{
    AR_PROFILE_FUNCTION();

    Texture* texture = GetTexture(texture_handle);

    if (texture->Mips == 1)
    {
        return;
    }

    VkImageAspectFlags aspect_mask = GetAspectFlagsFromFormat(texture->Info.Format);

    VkImageMemoryBarrier pre_barrier_base = {};
    pre_barrier_base.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    pre_barrier_base.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    pre_barrier_base.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    pre_barrier_base.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    pre_barrier_base.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    pre_barrier_base.image = texture->Image;
    pre_barrier_base.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pre_barrier_base.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pre_barrier_base.subresourceRange.aspectMask = aspect_mask;
    pre_barrier_base.subresourceRange.baseMipLevel = 0;
    pre_barrier_base.subresourceRange.baseArrayLayer = 0;
    pre_barrier_base.subresourceRange.layerCount = texture->Info.Layers;
    pre_barrier_base.subresourceRange.levelCount = 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &pre_barrier_base);

    VkImageMemoryBarrier pre_barrier = {};
    pre_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    pre_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    pre_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    pre_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    pre_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    pre_barrier.image = texture->Image;
    pre_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pre_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pre_barrier.subresourceRange.aspectMask = aspect_mask;
    pre_barrier.subresourceRange.baseMipLevel = 1;
    pre_barrier.subresourceRange.baseArrayLayer = 0;
    pre_barrier.subresourceRange.layerCount = texture->Info.Layers;
    pre_barrier.subresourceRange.levelCount = texture->Mips - 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &pre_barrier);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.image = texture->Image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = texture->Info.Layers;
    barrier.subresourceRange.levelCount = 1;

    int mip_width = texture->Info.Width;
    int mip_height = texture->Info.Height;
    int mip_depth = texture->Info.Depth;

    for (unsigned int i = 1; i < texture->Mips; i++)
    {
        VkImageBlit blit = {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mip_width, mip_height, 1};
        blit.srcSubresource.aspectMask = aspect_mask;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = texture->Info.Layers;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, mip_depth > 1 ? mip_depth / 2 : 1};
        blit.dstSubresource.aspectMask = aspect_mask;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = texture->Info.Layers;

        vkCmdBlitImage(cmd, texture->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.subresourceRange.baseMipLevel = i;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mip_width > 1)
            mip_width /= 2;
        if (mip_height > 1)
            mip_height /= 2;
        if (mip_depth > 1)
            mip_depth /= 2;
    }
}

void Device::DestroyTexture(RenderHandle texture_handle)
{
    AR_PROFILE_FUNCTION();

    Texture* texture = GetTexture(texture_handle);
    // If is swapchain image, we only want to destroy the image views
    if (!texture->IsSwapchainTexture)
    {
        GetFrame()->ImagesToDestroy.push_back(texture->Image);
        GetFrame()->MemoryToFree.push_back(texture->Memory);
    }
    for (auto& layer : texture->ImageViewLayers)
    {
        GetFrame()->ImageViewsToDestroy.push_back(layer);
    }
    for (auto& level : texture->ImageViewLevels)
    {
        GetFrame()->ImageViewsToDestroy.push_back(level);
    }
    GetFrame()->ImageViewsToDestroy.push_back(texture->ImageView);

    m_RenderHandleAllocator->Release(texture_handle);
    m_TextureStorage.Remove(texture_handle);
}

// Samplers

RenderHandle Device::CreateSampler(const SamplerInfo& info)
{
    AR_PROFILE_FUNCTION();

    VkSampler sampler;
    CreateRawSampler(&sampler, info.Filter, info.AddressMode, info.MipMode, info.Mips);

    Sampler resource;
    resource.Info = info;
    resource.SamplerRaw = sampler;

    RenderHandle render_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::Sampler);
    m_SamplerStorage.Add(render_handle, resource);

    return render_handle;
}

void Device::DestroySampler(RenderHandle sampler_handle)
{
    AR_PROFILE_FUNCTION();

    VkSampler sampler = GetSampler(sampler_handle)->SamplerRaw;

    GetFrame()->SamplersToDestroy.push_back(sampler);

    m_RenderHandleAllocator->Release(sampler_handle);
    m_SamplerStorage.Remove(sampler_handle);
}

// Shader modules

RenderHandle Device::CreateShaderModule(const ShaderModuleInfo& info)
{
    AR_PROFILE_FUNCTION();

    VkShaderModule shader_module;

    VkShaderModuleCreateInfo shader_ci = {};
    shader_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_ci.codeSize = info.Code.size();
    shader_ci.pCode = (uint32*)info.Code.data();

    VK_CALL(vkCreateShaderModule(m_Handle, &shader_ci, nullptr, &shader_module));

    ShaderModule resource;
    resource.Info = info;
    resource.ShaderModuleRaw = shader_module;

    RenderHandle render_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::ShaderModule);
    m_ShaderModuleStorage.Add(render_handle, resource);

    return render_handle;
}

void Device::DestroyShaderModule(RenderHandle shader_module_handle)
{
    AR_PROFILE_FUNCTION();

    VkShaderModule module = GetShaderModule(shader_module_handle)->ShaderModuleRaw;

    vkDestroyShaderModule(m_Handle, module, nullptr);

    m_RenderHandleAllocator->Release(shader_module_handle);
    m_ShaderModuleStorage.Remove(shader_module_handle);
}

// Pipeline layouts

RenderHandle Device::CreatePipelineLayout(const PipelineLayoutInfo& info)
{
    AR_PROFILE_FUNCTION();

    ScopedAllocator alloc;

    VkPushConstantRange* ranges = alloc.Allocate<VkPushConstantRange>(info.PushConstantRangeCount);
    VkDescriptorSetLayout* layouts = alloc.Allocate<VkDescriptorSetLayout>(info.DescriptorSetLayoutCount);

    for (uint32 i = 0; i < info.DescriptorSetLayoutCount; i++)
    {
        layouts[i] = m_DescriptorSetLayoutInternalCache.RequestPointer(info.DescriptorSetLayouts[i])->GetRaw();
    }

    for (uint32 i = 0; i < info.PushConstantRangeCount; i++)
    {
        const PushConstantRange& range = info.PushConstantRanges[i];
        ranges[i].stageFlags = RenderBackend::ConvertShaderStage(range.Stage);
        ranges[i].size = range.Size;
        ranges[i].offset = range.Offset;
    }

    VkPipelineLayout pipeline_layout;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = info.DescriptorSetLayoutCount;
    pipeline_layout_ci.pSetLayouts = layouts;
    pipeline_layout_ci.pushConstantRangeCount = info.PushConstantRangeCount;
    pipeline_layout_ci.pPushConstantRanges = ranges;

    VK_CALL(vkCreatePipelineLayout(m_Handle, &pipeline_layout_ci, nullptr, &pipeline_layout));

    PipelineLayout resource;
    resource.Info = info;
    for (uint32 i = 0; i < info.DescriptorSetLayoutCount; i++)
    {
        resource.DescriptorSetLayouts[i] = layouts[i];
    }
    resource.PipelineLayoutRaw = pipeline_layout;

    RenderHandle render_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::PipelineLayout);
    m_PipelineLayoutStorage.Add(render_handle, resource);

    return render_handle;
}

void Device::DestroyPipelineLayout(RenderHandle pipeline_layout_handle)
{
    AR_PROFILE_FUNCTION();

    PipelineLayout* layout = GetPipelineLayout(pipeline_layout_handle);

    vkDestroyPipelineLayout(m_Handle, layout->PipelineLayoutRaw, nullptr);

    m_RenderHandleAllocator->Release(pipeline_layout_handle);
    m_PipelineLayoutStorage.Remove(pipeline_layout_handle);
}

RenderHandle Device::CreateRenderPass(const RenderPassInfo& info)
{
    AR_PROFILE_FUNCTION();

    ScopedAllocator alloc;

    uint32 color_count = info.Layout.ColorCount;
    uint32 resolve_count = info.Layout.ResolveCount;
    uint32 depth_count = info.Layout.DepthCount;
    uint32 attachment_count = color_count + resolve_count + depth_count;
    VkAttachmentDescription* attachment_descs = alloc.Allocate<VkAttachmentDescription>(attachment_count);

    VkAttachmentReference* color_refs = alloc.Allocate<VkAttachmentReference>(color_count);
    VkAttachmentReference* resolve_refs = alloc.Allocate<VkAttachmentReference>(resolve_count);
    VkAttachmentReference* depth_ref = alloc.Allocate<VkAttachmentReference>(depth_count);

    uint32 offset = 0;
    for (uint32 i = 0; i < color_count; i++)
    {
        VkAttachmentDescription& desc = attachment_descs[i];
        const RenderPassLayout::Attachment& layout = info.Layout.Colors[i];
        const RenderPassInfo::AttachmentOptions& options = info.ColorOptions[i];
        desc = {};
        desc.format = layout.Format;
        desc.samples = layout.Samples;
        desc.loadOp = RenderBackend::ConvertAttachmentLoadOptions(options.LoadOp);
        desc.storeOp = RenderBackend::ConvertAttachmentStoreOptions(options.StoreOp);
        // No stencil for now.
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        color_refs[i] = {offset + i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    }
    offset += color_count;
    for (uint32 i = 0; i < resolve_count; i++)
    {
        VkAttachmentDescription& desc = attachment_descs[offset + i];
        const RenderPassLayout::Attachment& layout = info.Layout.Colors[i];
        const RenderPassInfo::AttachmentOptions& options = info.ResolveOptions[i];
        desc = {};
        desc.format = layout.Format;
        desc.samples = VK_SAMPLE_COUNT_1_BIT;
        desc.loadOp = RenderBackend::ConvertAttachmentLoadOptions(options.LoadOp);
        desc.storeOp = RenderBackend::ConvertAttachmentStoreOptions(options.StoreOp);
        // No stencil for now.
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        resolve_refs[i] = {offset + i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    }
    offset += resolve_count;
    for (uint32 i = 0; i < depth_count; i++)
    {
        VkAttachmentDescription& desc = attachment_descs[offset + i];
        const RenderPassLayout::Attachment& layout = info.Layout.Depth[i];
        const RenderPassInfo::AttachmentOptions& options = info.DepthOptions[i];
        desc = {};
        desc.format = layout.Format;
        desc.samples = layout.Samples;
        desc.loadOp = RenderBackend::ConvertAttachmentLoadOptions(options.LoadOp);
        desc.storeOp = RenderBackend::ConvertAttachmentStoreOptions(options.StoreOp);
        // No stencil for now.
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depth_ref[i] = {offset + i, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    }

    VkRenderPass rp;
    CreateRawRenderPass(&rp, attachment_count, attachment_descs, color_count, color_refs, resolve_count ? resolve_refs : nullptr, depth_count ? depth_ref : nullptr);

    RenderPass resource;
    resource.Info = info;
    resource.RenderPassRaw = rp;

    RenderHandle render_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::RenderPass);
    m_RenderPassStorage.Add(render_handle, resource);

    return render_handle;
}

void Device::DestroyRenderPass(RenderHandle render_pass_handle)
{
    AR_PROFILE_FUNCTION();

    VkRenderPass render_pass = GetRenderPass(render_pass_handle)->RenderPassRaw;

    GetFrame()->RenderPassesToDestroy.push_back(render_pass);

    m_RenderHandleAllocator->Release(render_pass_handle);
    m_RenderPassStorage.Remove(render_pass_handle);
}

// Pipelines

RenderHandle Device::CreateGraphicsPipeline(GraphicsPipelineInfo& info)
{
    AR_PROFILE_FUNCTION();

    // Create vertex input state
    uint32 binding_count = info.VertexLayout.Bindings.size();
    uint32 attribute_count = 0;

    for (auto& block : info.VertexLayout.Bindings)
    {
        attribute_count += block.Attributes.size();
    }

    std::vector<VkVertexInputBindingDescription> vertex_bindings;
    vertex_bindings.reserve(binding_count);
    std::vector<VkVertexInputAttributeDescription> vertex_attributes;
    vertex_attributes.reserve(attribute_count);

    uint32 location = 0;
    for (uint32 i = 0; i < binding_count; i++)
    {
        uint32 offset = 0;

        for (auto& attribute : info.VertexLayout.Bindings[i].Attributes)
        {
            VkVertexInputAttributeDescription attribute_desc;
            attribute_desc.binding = i;
            attribute_desc.format = RenderBackend::ConvertFormat(attribute);
            attribute_desc.location = location;
            attribute_desc.offset = offset;

            vertex_attributes.push_back(attribute_desc);

            location += VertexLayout::GetFormatLocationSize(attribute);
            offset += VertexLayout::GetFormatSize(attribute);
        }

        VkVertexInputBindingDescription binding_desc;
        binding_desc.binding = i;
        binding_desc.stride = offset;
        binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // No instancing for now

        vertex_bindings.push_back(binding_desc);
    }

    // create pipeline

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 0.0f;
    viewport.height = 0.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {0, 0};

    VkPipelineViewportStateCreateInfo viewport_state_ci = {};
    viewport_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_ci.viewportCount = 1;
    viewport_state_ci.pViewports = &viewport;
    viewport_state_ci.scissorCount = 1;
    viewport_state_ci.pScissors = &scissor;

    VkPipelineShaderStageCreateInfo vert_stage_ci = {};
    vert_stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_ci.module = GetShaderModule(info.VertexShaderModule)->ShaderModuleRaw;
    vert_stage_ci.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_ci = {};
    frag_stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_ci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_ci.module = GetShaderModule(info.FragmentShaderModule)->ShaderModuleRaw;
    frag_stage_ci.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_ci, frag_stage_ci};

    VkPipelineVertexInputStateCreateInfo vertex_input_ci = {};
    vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_ci.vertexBindingDescriptionCount = vertex_bindings.size();
    vertex_input_ci.pVertexBindingDescriptions = vertex_bindings.data();
    vertex_input_ci.vertexAttributeDescriptionCount = vertex_attributes.size();
    vertex_input_ci.pVertexAttributeDescriptions = vertex_attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {};
    input_assembly_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_ci.topology = info.PrimitiveType;
    input_assembly_ci.primitiveRestartEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterizer_ci = {};
    rasterizer_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_ci.depthClampEnable = VK_FALSE;
    rasterizer_ci.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_ci.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_ci.lineWidth = 1.0f;
    rasterizer_ci.cullMode = info.CullMode;
    rasterizer_ci.frontFace = info.FrontFace;
    rasterizer_ci.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling_ci = {};
    multisampling_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_ci.sampleShadingEnable = VK_FALSE;
    multisampling_ci.rasterizationSamples = info.Samples;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_ci = {};
    depth_stencil_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_ci.depthTestEnable = info.DepthTestEnabled ? VK_TRUE : VK_FALSE;
    depth_stencil_ci.depthWriteEnable = info.DepthWriteEnabled ? VK_TRUE : VK_FALSE;
    depth_stencil_ci.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_ci.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_ci.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = info.BlendEnabled ? VK_TRUE : VK_FALSE;

    if (info.AdditiveBlendEnabled)
    {
        // None of this color blending stuff for now
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    else
    {
        // None of this color blending stuff for now
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    std::vector<VkPipelineColorBlendAttachmentState> blends;
    uint32 blend_count = info.RenderPassLayout.ColorCount;
    for (uint32 i = 0; i < blend_count; i++)
    {
        blends.push_back(color_blend_attachment);
    }

    VkPipelineColorBlendStateCreateInfo color_blend_ci = {};
    color_blend_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    //color_blend_ci.logicOpEnable = VK_TRUE;
    //color_blend_ci.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blend_ci.attachmentCount = blend_count;
    color_blend_ci.pAttachments = blends.data();

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state_ci = {};
    dynamic_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_ci.dynamicStateCount = 2;
    dynamic_state_ci.pDynamicStates = dynamic_states;

    VkGraphicsPipelineCreateInfo pipeline_ci = {};
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.stageCount = 2;
    pipeline_ci.pStages = shader_stages;
    pipeline_ci.pVertexInputState = &vertex_input_ci;
    pipeline_ci.pInputAssemblyState = &input_assembly_ci;
    pipeline_ci.pViewportState = &viewport_state_ci;
    pipeline_ci.pRasterizationState = &rasterizer_ci;
    pipeline_ci.pMultisampleState = &multisampling_ci;
    pipeline_ci.pDepthStencilState = &depth_stencil_ci;
    pipeline_ci.pColorBlendState = &color_blend_ci;
    pipeline_ci.pDynamicState = &dynamic_state_ci;

    pipeline_ci.layout = GetPipelineLayout(info.PipelineLayout)->PipelineLayoutRaw;
    pipeline_ci.subpass = 0;

    // Create dummy render pass for pipeline
    RenderPassInfo dummy;
    dummy.Layout = info.RenderPassLayout;
    for (auto& op : dummy.ColorOptions)
    {
        op = RenderPassInfo::AttachmentOptions();
    }
    for (auto& op : dummy.ResolveOptions)
    {
        op = RenderPassInfo::AttachmentOptions();
    }
    for (auto& op : dummy.DepthOptions)
    {
        op = RenderPassInfo::AttachmentOptions();
    }

    RenderHandle dummy_rp = CreateRenderPass(dummy);
    VkRenderPass rp = GetRenderPass(dummy_rp)->RenderPassRaw;

    pipeline_ci.renderPass = rp;

    VkPipeline pipeline;
    VK_CALL(vkCreateGraphicsPipelines(m_Handle, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &pipeline));

    // Destroy temp renderpass for pipeline creation
    DestroyRenderPass(dummy_rp);

    GraphicsPipeline resource;
    resource.Info = info;
    resource.PipelineRaw = pipeline;

    RenderHandle render_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::GraphicsPipeline);
    m_GraphicsPipelineStorage.Add(render_handle, resource);

    return render_handle;
}

void Device::DestroyGraphicsPipeline(RenderHandle graphics_pipeline_handle)
{
    AR_PROFILE_FUNCTION();

    VkPipeline pipeline = GetGraphicsPipeline(graphics_pipeline_handle)->PipelineRaw;

    GetFrame()->PipelinesToDestroy.push_back(pipeline);

    m_RenderHandleAllocator->Release(graphics_pipeline_handle);
    m_GraphicsPipelineStorage.Remove(graphics_pipeline_handle);
}

RenderHandle Device::CreateComputePipeline(ComputePipelineInfo& info)
{
    AR_PROFILE_FUNCTION();

    VkPipelineShaderStageCreateInfo shader_stage_ci = {};
    shader_stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_ci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage_ci.module = GetShaderModule(info.ComputeShaderModule)->ShaderModuleRaw;
    shader_stage_ci.pName = "main";

    VkComputePipelineCreateInfo pipeline_ci = {};
    pipeline_ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_ci.stage = shader_stage_ci;
    pipeline_ci.layout = GetPipelineLayout(info.PipelineLayout)->PipelineLayoutRaw;

    VkPipeline pipeline;
    VK_CALL(vkCreateComputePipelines(m_Handle, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &pipeline));

    ComputePipeline resource;
    resource.Info = info;
    resource.PipelineRaw = pipeline;

    RenderHandle render_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::ComputePipeline);
    m_ComputePipelineStorage.Add(render_handle, resource);

    return render_handle;
}
void Device::DestroyComputePipeline(RenderHandle compute_pipeline_handle)
{
    AR_PROFILE_FUNCTION();

    VkPipeline pipeline = GetComputePipeline(compute_pipeline_handle)->PipelineRaw;

    GetFrame()->PipelinesToDestroy.push_back(pipeline);

    m_RenderHandleAllocator->Release(compute_pipeline_handle);
    m_ComputePipelineStorage.Remove(compute_pipeline_handle);
}

// Framebuffers
RenderHandle Device::CreateFramebuffer(const FramebufferInfo& info)
{
    AR_PROFILE_FUNCTION();

    ScopedAllocator alloc;

    RenderPassLayout layout = info.RenderPassLayout;
    uint32 attachment_count = layout.ColorCount + layout.ResolveCount + layout.DepthCount;
    VkImageView* attachments = alloc.Allocate<VkImageView>(attachment_count);
    uint32 offset = 0;
    for (uint32 i = 0; i < layout.ColorCount; i++)
    {
        int32 layer = info.Layers[i];
        if (layer == -1)
        {
            attachments[offset + i] = GetTexture(info.ColorTextures[i])->ImageView;
        }
        else
        {
            attachments[offset + i] = GetTexture(info.ColorTextures[i])->ImageViewLayers[layer];
        }
    }
    offset += layout.ColorCount;
    for (uint32 i = 0; i < layout.ResolveCount; i++)
    {
        attachments[offset + i] = GetTexture(info.ResolveTextures[i])->ImageView;
    }
    offset += layout.ResolveCount;
    for (uint32 i = 0; i < layout.DepthCount; i++)
    {
        attachments[offset + i] = GetTexture(info.DepthTexture[i])->ImageView;
    }

    // Create dummy render pass for pipeline
    RenderPassInfo dummy;
    dummy.Layout = info.RenderPassLayout;
    for (auto& op : dummy.ColorOptions)
    {
        op = RenderPassInfo::AttachmentOptions();
    }
    for (auto& op : dummy.ResolveOptions)
    {
        op = RenderPassInfo::AttachmentOptions();
    }
    for (auto& op : dummy.DepthOptions)
    {
        op = RenderPassInfo::AttachmentOptions();
    }

    RenderHandle dummy_rp = CreateRenderPass(dummy);
    VkRenderPass rp = GetRenderPass(dummy_rp)->RenderPassRaw;

    VkFramebufferCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.renderPass = rp;
    ci.attachmentCount = attachment_count;
    ci.pAttachments = attachments;
    ci.width = info.Width;
    ci.height = info.Height;
    ci.layers = 1;

    VkFramebuffer framebuffer;
    VK_CALL(vkCreateFramebuffer(m_Handle, &ci, nullptr, &framebuffer));

    // Destroy dummy render pass
    DestroyRenderPass(dummy_rp);

    Framebuffer resource;
    resource.Info = info;
    resource.FramebufferRaw = framebuffer;

    RenderHandle render_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::Framebuffer);
    m_FramebufferStorage.Add(render_handle, resource);

    return render_handle;
}

void Device::DestroyFramebuffer(RenderHandle framebuffer_handle)
{
    AR_PROFILE_FUNCTION();

    VkFramebuffer framebuffer = GetFramebuffer(framebuffer_handle)->FramebufferRaw;
    GetFrame()->FramebuffersToDestroy.push_back(framebuffer);

    m_RenderHandleAllocator->Release(framebuffer_handle);
    m_FramebufferStorage.Remove(framebuffer_handle);
}

// Descriptor sets

RenderHandle Device::CreateDescriptorSet(const DescriptorSetLayout& layout)
{
    AR_PROFILE_FUNCTION();

    DescriptorSetLayoutInternal* internal = m_DescriptorSetLayoutInternalCache.RequestPointer(layout);

    DescriptorSet resource;
    resource.DescriptorSetLayout = layout;
    resource.DescriptorSetRaw = internal->AllocateDescriptorSet();

    RenderHandle render_handle = m_RenderHandleAllocator->Allocate(RenderHandleType::DescriptorSet);
    m_DescriptorSetStorage.Add(render_handle, resource);

    return render_handle;
}

void Device::UpdateDescriptorSet(RenderHandle descriptor_set, const DescriptorSetUpdate& info)
{
    AR_PROFILE_FUNCTION();

    ScopedAllocator alloc;

    uint32 write_count = info.Descriptors.size();

    VkWriteDescriptorSet* writes = alloc.Allocate<VkWriteDescriptorSet>(write_count);

    for (uint32 i = 0; i < write_count; i++)
    {
        const DescriptorSetUpdate::Descriptor& desc = info.Descriptors[i];
        writes[i] = {};
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = GetDescriptorSet(descriptor_set)->DescriptorSetRaw;
        writes[i].dstBinding = desc.Binding;
        writes[i].dstArrayElement = desc.ArrayOffset;
        writes[i].descriptorType = RenderBackend::ConvertDescriptorType(desc.Type);
        writes[i].descriptorCount = desc.Count;

        VkDescriptorImageInfo* image_infos = alloc.Allocate<VkDescriptorImageInfo>(desc.Count);
        VkDescriptorBufferInfo* buffer_infos = alloc.Allocate<VkDescriptorBufferInfo>(desc.Count);

        for (uint32 j = 0; j < desc.Count; j++)
        {
            switch (RenderBackend::ConvertDescriptorType(desc.Type))
            {
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                image_infos[j].sampler = GetSampler(desc.ImageInfos[j].Sampler)->SamplerRaw;
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                VkImageView view;
                if (desc.ImageInfos[j].TextureLevel >= 0)
                    view = GetTexture(desc.ImageInfos[j].Texture)->ImageViewLevels[desc.ImageInfos[j].TextureLevel];
                else
                    view = GetTexture(desc.ImageInfos[j].Texture)->ImageView;

                image_infos[j].imageView = view;
                image_infos[j].imageLayout = RenderBackend::ConvertRenderBindFlagToImageLayout(desc.ImageInfos[j].BindFlag);
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                buffer_infos[j].buffer = GetBuffer(desc.BufferInfos[j].Buffer)->BufferRaw;
                buffer_infos[j].offset = desc.BufferInfos[j].Offset;
                buffer_infos[j].range = desc.BufferInfos[j].Range;
                break;
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            default:
                AR_CORE_FATAL("Used unsupported descriptor type!");
            }

        }
        writes[i].pBufferInfo = buffer_infos;
        writes[i].pImageInfo = image_infos;
    }

    vkUpdateDescriptorSets(m_Handle, write_count, writes, 0, nullptr);
}


void Device::DestroyDescriptorSet(RenderHandle descriptor_set_handle)
{
    AR_PROFILE_FUNCTION();

    DescriptorSet* descriptor_set = GetDescriptorSet(descriptor_set_handle);

    GetFrame()->DescriptorSetsToRelease.push_back(*descriptor_set);

    m_RenderHandleAllocator->Release(descriptor_set_handle);
    m_DescriptorSetStorage.Remove(descriptor_set_handle);
}

// Helpers to create raw vulkan objects

void Device::CreateRawBuffer(VkBuffer* buffer, VkDeviceMemory* memory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties)
{
    AR_PROFILE_FUNCTION();

    // Create buffer
    VkBufferCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size = size;
    ci.usage = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CALL(vkCreateBuffer(m_Handle, &ci, nullptr, buffer));

    // Allocate memory for buffer
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(m_Handle, *buffer, &memory_requirements);

    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memory_requirements.size;
    ai.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, memory_properties);
    VK_CALL(vkAllocateMemory(m_Handle, &ai, nullptr, memory));

    // Bind memory to buffer
    VK_CALL(vkBindBufferMemory(m_Handle, *buffer, *memory, 0));
    std::string name;
}

void Device::CreateRawImage(VkImage* image, VkDeviceMemory* memory, VkImageType type, uint32 width, uint32 height, uint32 depth, uint32 mips, uint32 layers, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples, VkFlags flags, VkMemoryPropertyFlags memory_properties)
{
    AR_PROFILE_FUNCTION();

    // Create image
    VkImageCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = type;
    ci.extent.width = width;
    ci.extent.height = height;
    ci.extent.depth = depth;
    ci.mipLevels = mips;
    ci.arrayLayers = layers;
    ci.format = format;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ci.usage = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.samples = samples;
    ci.flags = flags;
    VK_CALL(vkCreateImage(m_Handle, &ci, nullptr, image));

    // Allocate memory for image
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(m_Handle, *image, &memory_requirements);

    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memory_requirements.size;
    ai.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, memory_properties);
    VK_CALL(vkAllocateMemory(m_Handle, &ai, nullptr, memory));

    // Bind memory to image
    vkBindImageMemory(m_Handle, *image, *memory, 0);
}

void Device::CreateRawImageView(VkImageView* view, VkImage image, VkImageViewType type, VkFormat format, uint32 base_mip, uint32 mips, uint32 base_layer, uint32 layers)
{
    AR_PROFILE_FUNCTION();

    VkImageViewCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.image = image;
    ci.viewType = type;
    ci.format = format;
    ci.subresourceRange.aspectMask = GetAspectFlagsFromFormat(format);
    ci.subresourceRange.baseMipLevel = base_mip;
    ci.subresourceRange.levelCount = mips;
    ci.subresourceRange.baseArrayLayer = base_layer;
    ci.subresourceRange.layerCount = layers;

    VK_CALL(vkCreateImageView(m_Handle, &ci, nullptr, view));
}

void Device::CreateRawSampler(VkSampler* sampler, VkFilter filter, VkSamplerAddressMode mode, VkSamplerMipmapMode mip_mode, uint32 mips)
{
    AR_PROFILE_FUNCTION();

    VkSamplerCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci.magFilter = filter;
    ci.minFilter = filter;
    ci.addressModeU = mode;
    ci.addressModeV = mode;
    ci.addressModeW = mode;
    ci.anisotropyEnable = VK_TRUE;
    ci.maxAnisotropy = 16;
    ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    ci.unnormalizedCoordinates = VK_FALSE;
    ci.compareEnable = VK_FALSE;
    ci.compareOp = VK_COMPARE_OP_ALWAYS;
    ci.mipmapMode = mip_mode;
    ci.mipLodBias = 0.0f;
    ci.minLod = 0;
    ci.maxLod = (float)mips;

    VK_CALL(vkCreateSampler(m_Handle, &ci, nullptr, sampler));
}

void Device::CreateRawRenderPass(VkRenderPass* render_pass, uint32 attachment_count, VkAttachmentDescription* attachments, uint32 color_count, VkAttachmentReference* color_refs, VkAttachmentReference* resolve_refs, VkAttachmentReference* depth_ref)
{
    AR_PROFILE_FUNCTION();

    VkSubpassDescription subpass_desc = {};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = nullptr;
    subpass_desc.colorAttachmentCount = color_count;
    subpass_desc.pColorAttachments = color_refs;
    subpass_desc.pResolveAttachments = resolve_refs;
    subpass_desc.pDepthStencilAttachment = depth_ref;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount = attachment_count;
    ci.pAttachments = attachments;
    ci.subpassCount = 1;
    ci.pSubpasses = &subpass_desc;
    ci.dependencyCount = 0;
    ci.pDependencies = nullptr;

    VK_CALL(vkCreateRenderPass(m_Handle, &ci, nullptr, render_pass));
}
