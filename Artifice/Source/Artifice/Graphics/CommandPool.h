#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Artifice/Core/Core.h"


class CommandPool
{
private:
    std::vector<VkCommandBuffer> m_CommandBuffers;
    VkDevice m_Device;
    VkCommandPool m_Handle;
    uint32 m_Index = 0;
public:
    CommandPool();
    void Init(VkDevice device, uint32 queue_family_index);
    void CleanUp();

    VkCommandBuffer GetCommandBuffer();
    void Reset();

    inline VkCommandPool GetHandle() const { return m_Handle; }
};