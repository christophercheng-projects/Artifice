#include "Device.h"

#include <set>

#include "Artifice/Core/Window.h"

#include <GLFW/glfw3.h>

#include "Artifice/math/math_functions.h"

#include "Artifice/Core/Application.h"




VkSurfaceFormatKHR Device::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats)
{
    //ideal
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        VkSurfaceFormatKHR result;
        result.format = VK_FORMAT_B8G8R8A8_UNORM;
        result.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        AR_CORE_INFO("Format: VK_FORMAT_B8G8R8A8_UNORM, Color space: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
        return result;
    }
    //some requirements
    for (unsigned int i = 0; i < formats.size(); i++)
    {
        const VkSurfaceFormatKHR &format = formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            AR_CORE_INFO("Format: VK_FORMAT_B8G8R8A8_UNORM, Color space: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
            return format;
        }
    }
    //last resort
    AR_CORE_INFO("Returned first available surface format");
    return formats[0];
}

VkPresentModeKHR Device::ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes)
{
    if (m_VSync)
    {
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    
    for (unsigned int i = 0; i < modes.size(); i++)
    {
        if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            AR_CORE_INFO("Present mode: VK_PRESENT_MODE_MAILBOX_KHR (uncapped)");
            return modes[i];
        }
        if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            AR_CORE_INFO("Present mode: VK_PRESENT_MODE_IMMEDIATE_KHR (uncapped)");
            return modes[i];
        }
    }

    AR_CORE_INFO("Present mode: VK_PRESENT_MODE_FIFO_KHR (v-sync)");
    return VK_PRESENT_MODE_FIFO_KHR; //v-sync is guaranteed
}

VkExtent2D Device::ChooseSurfaceExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32 width, uint32 height)
{
    VkExtent2D extent;

    if (capabilities.currentExtent.width == -1)
    {
        extent.width = width;
        extent.height = height;
    }
    else
    {
        extent = capabilities.currentExtent;
    }

    return extent;
}

uint32 Device::FindMemoryType(uint32 type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice.Handle, &memory_properties);

    for (uint32 i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    AR_CORE_FATAL("Unable to find requested memory type!");
}

VkImageUsageFlags Device::GetAttachmentUsageFromFormat(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_S8_UINT:
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_X8_D24_UNORM_PACK32:
                return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            default:
                return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
    }

VkPipelineStageFlags Device::GetStagesFromImageUsage(VkImageUsageFlags usage)
{
	VkPipelineStageFlags flags = 0;

	if (usage & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
	if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
		flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
		         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
		flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
		flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	if (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
	{
		VkPipelineStageFlags possible = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		                                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
		                                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
			possible |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		flags &= possible;
	}

	return flags;
}

VkAccessFlags Device::GetAccessFromImageUsage(VkImageUsageFlags usage)
{
	VkAccessFlags flags = 0;

	if (usage & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		flags |= VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
		flags |= VK_ACCESS_SHADER_READ_BIT;
	if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
		flags |= VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
		flags |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

	if (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
	{
		flags &= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		         VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	}

	return flags;
}

VkAccessFlags Device::GetAccessFromImageLayout(VkImageLayout layout)
{
	switch (layout)
	{
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		return VK_ACCESS_TRANSFER_READ_BIT;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return VK_ACCESS_TRANSFER_WRITE_BIT;
	default:
		return 0;
	}
}

VkImageAspectFlags Device::GetAspectFlagsFromFormat(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_UNDEFINED:
		    return 0;
        // Stencil only
        case VK_FORMAT_S8_UINT:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        // Depth and stencil
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;
        // Depth only
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        // Otherwise color
        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

bool Device::IsImageFormatSupported(VkFormat format, VkFormatFeatureFlags features, VkImageTiling tiling)
{
    VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice.Handle, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
    {
        return true;
    }
    else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
    {
        return true;
    }

    return false;
}

VkFormat Device::GetDefaultDepthFormat()
{
    if (IsImageFormatSupported(VK_FORMAT_D32_SFLOAT, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL));
    {
        return VK_FORMAT_D32_SFLOAT;
    }
    if (IsImageFormatSupported(VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL));
    {
        return VK_FORMAT_X8_D24_UNORM_PACK32;
    }
    if (IsImageFormatSupported(VK_FORMAT_D16_UNORM, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL));
    {
        return VK_FORMAT_D16_UNORM;
    }

    AR_CORE_FATAL("No supported depth formats found!");
}

void Device::SelectPhysicalDevice()
{
    GLFWwindow* window = glfwCreateWindow(1, 1, "Dummy", nullptr, nullptr);
    
    VkSurfaceKHR surface;
    VK_CALL(glfwCreateWindowSurface(m_Instance, window, nullptr, &surface));
    
    unsigned int deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
    if (deviceCount <= 0)
    {
        AR_CORE_INFO("Failed to find GPU with Vulkan support");
    }
    else
    {
        AR_CORE_INFO("Found %d GPU(s) with Vulkan support!", deviceCount);
    }
    std::vector<VkPhysicalDevice> devices;
    devices.resize(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

    std::vector<PhysicalDevice> gpus;
    gpus.resize(deviceCount);

    //load all info for each gpu into vector
    for (unsigned int i = 0; i < deviceCount; ++i)
    {
        PhysicalDevice &gpu = gpus[i];
        gpu.Handle = devices[i];
        //get device queue family properties
        {
            unsigned int queueCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(gpu.Handle, &queueCount, nullptr);
            AR_CORE_ASSERT(queueCount > 0, "No queues found");

            gpu.QueueFamilyProperties.resize(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(gpu.Handle, &queueCount, gpu.QueueFamilyProperties.data());
            AR_CORE_ASSERT(queueCount > 0, "No queues found");
        }
        //get device extension properties
        {
            unsigned int extensionCount = 0;
            vkEnumerateDeviceExtensionProperties(gpu.Handle, nullptr, &extensionCount, nullptr);
            AR_CORE_ASSERT(extensionCount > 0, "No extensions found");

            gpu.ExtensionProperties.resize(extensionCount);
            vkEnumerateDeviceExtensionProperties(gpu.Handle, nullptr, &extensionCount, gpu.ExtensionProperties.data());
            AR_CORE_ASSERT(extensionCount > 0, "No extensions found");

            AR_CORE_INFO("Avialable device extensions: ");
            for (unsigned int i = 0; i < gpu.ExtensionProperties.size(); i++)
            {
                AR_CORE_INFO("\t%s", gpu.ExtensionProperties[i].extensionName);
            }
        }

        //get surface capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.Handle, surface, &gpu.SurfaceCapabilities);
        //get surface formats
        {
            unsigned int formatCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.Handle, surface, &formatCount, nullptr);
            AR_CORE_ASSERT(formatCount > 0, "No surface formats found");

            gpu.SurfaceFormats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.Handle, surface, &formatCount, gpu.SurfaceFormats.data());
            AR_CORE_ASSERT(formatCount > 0, "No surface formats found");
        }
        //get surface present modes
        {
            unsigned int presentModesCount = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.Handle, surface, &presentModesCount, nullptr);
            AR_CORE_ASSERT(presentModesCount > 0, "No present modes found");

            gpu.PresentModes.resize(presentModesCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.Handle, surface, &presentModesCount, gpu.PresentModes.data());
            AR_CORE_ASSERT(presentModesCount > 0, "No present modes found");
        }
        vkGetPhysicalDeviceMemoryProperties(gpu.Handle, &gpu.MemoryProperties);
        vkGetPhysicalDeviceProperties(gpu.Handle, &gpu.Properties);
        vkGetPhysicalDeviceFeatures(gpu.Handle, &gpu.Features);
    } //end enumerating gpus

    //select device
    for (unsigned int i = 0; i < gpus.size(); i++)
    {
        PhysicalDevice gpu = gpus[i];

        if (!CheckDeviceExtensionSupport(gpu, m_DeviceExtensions))
        {
            continue;
        }

        AR_CORE_INFO("Enabling device extensions: ");
        for (unsigned int j = 0; j < m_DeviceExtensions.size(); j++)
        {
            AR_CORE_INFO("\t%s", m_DeviceExtensions[j]);
        }

        if (gpu.SurfaceFormats.size() == 0)
        {
            continue;
        }

        if (gpu.PresentModes.size() == 0)
        {
            continue;
        }

        if (gpu.Features.samplerAnisotropy == VK_FALSE)
        {
            continue;
        }

        int universal_index = -1;
        uint32 universal_count = 0;
        bool universal_present = false;
        // Find universal queue family
        for (int j = 0; j < gpu.QueueFamilyProperties.size(); j++)
        {
            VkQueueFamilyProperties &properties = gpu.QueueFamilyProperties[j];

            if (properties.queueCount <= 0)
            {
                continue;
            }

            // Select a queue that supports both graphics and compute.
            // This is guaranteed by the spec chapter 4.1
            // "If an implementation exposes any queue family that supports graphics operations, 
            //  at least one queue family of at least one physical device exposed by the implementation 
            //  must support both graphics and compute operations."
            // The sepc also guarantees that a queue that supports either graphics or compute must also support transfers
            // So we are selecting a universal queue.

            if ((properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (properties.queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                universal_index = j;
                universal_count = properties.queueCount;
                break;
            }
        }

        int compute_index = -1;
        uint32 compute_count = 0;
        bool compute_present = false;
        // Find compute queue family
        for (int j = 0; j < gpu.QueueFamilyProperties.size(); j++)
        {
            VkQueueFamilyProperties& properties = gpu.QueueFamilyProperties[j];

            if (properties.queueCount <= 0)
            {
                continue;
            }

            if (properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                if (j == universal_index)
                {
                    continue;
                }
                compute_index = j;
                compute_count = properties.queueCount;
                break;
            }
        }

        int present_index = -1;
        // Find present queue family

        for (int j = 0; j < gpu.QueueFamilyProperties.size(); j++)
        {
            VkQueueFamilyProperties &properties = gpu.QueueFamilyProperties[j];

            if (properties.queueCount <= 0)
            {
                continue;
            }
            VkBool32 supports_present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu.Handle, j, surface, &supports_present);
            if (supports_present && j == universal_index)
            {
                present_index = j;
                universal_present = true;
                break;
            }
        }
        for (int j = 0; j < gpu.QueueFamilyProperties.size(); j++)
        {
            VkQueueFamilyProperties &properties = gpu.QueueFamilyProperties[j];

            if (properties.queueCount <= 0)
            {
                continue;
            }
            VkBool32 supports_present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu.Handle, j, surface, &supports_present);
            if (supports_present && j == compute_index)
            {
                compute_present = true;
                break;
            }
        }
        if (present_index == -1)
        {
            AR_CORE_WARN("Universal queue does not support present. Present will be on a separate queue. Caution: additional synchronization required");
            for (int j = 0; j < gpu.QueueFamilyProperties.size(); j++)
            {
                VkQueueFamilyProperties &properties = gpu.QueueFamilyProperties[j];

                if (properties.queueCount <= 0)
                {
                    continue;
                }
                VkBool32 supports_present = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(gpu.Handle, j, surface, &supports_present);
                if (supports_present)
                {
                    present_index = j;
                    break;
                }
            }
        }

        if (universal_index >= 0 && present_index >= 0)
        {
            m_UniversalFamilyIndex = universal_index;
            if (present_index != universal_index)
            {
                m_PresentFamilyIndex = present_index;
            }

            if (universal_present)
            {
                m_UniversalQueuePresentSupported = true;
            }

            if (compute_index >= 0)
            {
                m_ComputeFamilyIndex = compute_index;
                if (compute_present)
                {
                    m_ComputeQueuePresentSupported = true;
                }
            }
            else if (universal_count > 1)
            {
                m_ComputeFamilyIndex = universal_index;
                if (universal_present)
                {
                    m_ComputeQueuePresentSupported = true;
                }
            }
            else
            {
                AR_CORE_WARN("No async compute queues available")
            }
            

            VkSampleCountFlags counts = math::min(gpu.Properties.limits.framebufferColorSampleCounts, gpu.Properties.limits.framebufferDepthSampleCounts);
            if (counts & VK_SAMPLE_COUNT_64_BIT)
            {
                m_MaxMSAASamples = VK_SAMPLE_COUNT_64_BIT;
            }
            else if (counts & VK_SAMPLE_COUNT_32_BIT)
            {
                m_MaxMSAASamples = VK_SAMPLE_COUNT_32_BIT;
            }
            else if (counts & VK_SAMPLE_COUNT_16_BIT)
            {
                m_MaxMSAASamples = VK_SAMPLE_COUNT_16_BIT;
            }
            else if (counts & VK_SAMPLE_COUNT_8_BIT)
            {
                m_MaxMSAASamples = VK_SAMPLE_COUNT_8_BIT;
            }
            else if (counts & VK_SAMPLE_COUNT_4_BIT)
            {
                m_MaxMSAASamples = VK_SAMPLE_COUNT_4_BIT;
            }
            else if (counts & VK_SAMPLE_COUNT_2_BIT)
            {
                m_MaxMSAASamples = VK_SAMPLE_COUNT_2_BIT;
            }
            else
            {
                m_MaxMSAASamples = VK_SAMPLE_COUNT_1_BIT;
            }

            m_PhysicalDevice = gpu;

            AR_CORE_INFO("Suitable GPU found!");

            // Destroy dummy window/surface
            vkDestroySurfaceKHR(m_Instance, surface, nullptr);
            glfwDestroyWindow(window);

            return;
        }
    } //end select device

    AR_CORE_FATAL("No suitable GPU found");
}

void Device::CreateLogicalDevice()
{
    std::set<int32> uniqueIndices;
    uniqueIndices.insert(m_UniversalFamilyIndex);
    if (m_PresentFamilyIndex >= 0)
    {
        uniqueIndices.insert(m_PresentFamilyIndex);
    }
    if (m_ComputeFamilyIndex >= 0)
    {
        uniqueIndices.insert(m_ComputeFamilyIndex);
    }


    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueIndices.size());

    float priority[2] = {1.0f, 1.0f};
    for (int32 index : uniqueIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = index;
        queueCreateInfo.queueCount = m_UniversalFamilyIndex == m_ComputeFamilyIndex ? 2 : 1;
        queueCreateInfo.pQueuePriorities = priority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    //create logical device
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = m_DeviceExtensions.size();
    createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

    VK_CALL(vkCreateDevice(m_PhysicalDevice.Handle, &createInfo, nullptr, &m_Handle));

    vkGetDeviceQueue(m_Handle, m_UniversalFamilyIndex, 0, &m_InternalUniversalQueue.Queue);
    m_UniversalQueue = &m_InternalUniversalQueue;

    if (m_ComputeFamilyIndex >= 0)
    {
        if (m_ComputeFamilyIndex == m_UniversalFamilyIndex)
        {
            vkGetDeviceQueue(m_Handle, m_ComputeFamilyIndex, 1, &m_InternalComputeQueue.Queue);
        }
        else
        {

            vkGetDeviceQueue(m_Handle, m_ComputeFamilyIndex, 0, &m_InternalComputeQueue.Queue);
        }
        m_ComputeQueue = &m_InternalComputeQueue;
    }

    if (m_PresentFamilyIndex >= 0)
    {
        vkGetDeviceQueue(m_Handle, m_PresentFamilyIndex, 0, &m_InternalPresentQueue.Queue);
        m_PresentQueue = &m_InternalPresentQueue;
    } // otherwise, present from univeral is supported
}

bool Device::CheckDeviceExtensionSupport(PhysicalDevice& gpu, const std::vector<const char *>& requiredExtensions)
{
    unsigned int required = (unsigned int)requiredExtensions.size();
    unsigned int available = 0;

    for (unsigned int i = 0; i < requiredExtensions.size(); i++)
    {
        for (unsigned int j = 0; j < gpu.ExtensionProperties.size(); j++)
        {
            if (strcmp(requiredExtensions[i], gpu.ExtensionProperties[j].extensionName) == 0)
            {
                available++;
                break;
            }
        }
    }

    return available == required;
}

#if 0

    {
        // Record stuff that needs the backbuffer. Is there a good way to synchronize this?
        // Submit the goods.
        std::vector<VkImageMemoryBarrier> to_transfer_dst_barriers;
        for (auto image : images)
        {
            VkImageMemoryBarrier ib = {};
            ib.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            ib.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            ib.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            ib.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            ib.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            ib.image = image;
            ib.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ib.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ib.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ib.subresourceRange.baseMipLevel = 0;
            ib.subresourceRange.baseArrayLayer = 0;
            ib.subresourceRange.layerCount = 1;
            ib.subresourceRange.levelCount = 1;

            to_transfer_dst_barriers.push_back(ib);
        }
        std::vector<VkImageMemoryBarrier> to_present_barriers;
        for (auto image : images)
        {
            VkImageMemoryBarrier ib = {};
            ib.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            ib.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            ib.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            ib.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            ib.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            ib.image = image;
            ib.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ib.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ib.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ib.subresourceRange.baseMipLevel = 0;
            ib.subresourceRange.baseArrayLayer = 0;
            ib.subresourceRange.layerCount = 1;
            ib.subresourceRange.levelCount = 1;

            to_present_barriers.push_back(ib);
        }

        VkCommandBuffer cmd = m_Frames[m_FrameIndex].GeneralCommandPool.GetCommandBuffer();

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CALL(vkBeginCommandBuffer(cmd, &begin_info));
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, to_transfer_dst_barriers.size(), to_transfer_dst_barriers.data());

        VkClearColorValue clear = {{1.0f, 0.8f, 0.4f, 1.0f}};
        VkImageSubresourceRange range = {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseArrayLayer = 0;
        range.layerCount = 1;
        range.baseMipLevel = 0;
        range.levelCount = 1;

        for (auto image : images)
        {
            vkCmdClearColorImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);
        }

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, to_present_barriers.size(), to_present_barriers.data());

        VK_CALL(vkEndCommandBuffer(cmd));

        // Submit
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = image_available_semaphores.size();
        submit_info.pWaitSemaphores = image_available_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stage_masks.data();
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd;
        submit_info.signalSemaphoreCount = swapchain_count;
        submit_info.pSignalSemaphores = render_finished_semaphores;

        VkFence fence = m_Frames[m_FrameIndex].GeneralFencePool.GetFence(m_Handle);

        VK_CALL(vkQueueSubmit(m_GeneralQueue, 1, &submit_info, fence));
    }
#endif
