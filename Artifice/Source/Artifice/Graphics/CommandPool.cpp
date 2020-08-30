#include "CommandPool.h"

#include "Artifice/Core/Log.h"


CommandPool::CommandPool()
{

}

void CommandPool::Init(VkDevice device, uint32 queue_family_index)
{
    m_Device = device;
    
    VkCommandPoolCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // We want to recreate command buffers every frame
    ci.queueFamilyIndex = queue_family_index;

    VK_CALL(vkCreateCommandPool(m_Device, &ci, nullptr, &m_Handle));
}

void CommandPool::CleanUp()
{
    vkDestroyCommandPool(m_Device, m_Handle, nullptr);
    m_CommandBuffers.clear();
}

VkCommandBuffer CommandPool::GetCommandBuffer()
{
    if (m_Index >= m_CommandBuffers.size())
    {
        VkCommandBuffer cmd;

        VkCommandBufferAllocateInfo ai = {};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = m_Handle;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Primary only for now
        ai.commandBufferCount = 1;

        VK_CALL(vkAllocateCommandBuffers(m_Device, &ai, &cmd));

        m_CommandBuffers.push_back(cmd);
    }

    return m_CommandBuffers[m_Index++];
}

void CommandPool::Reset()
{
    if (m_Index > 0)
    {
        m_Index = 0;
        VK_CALL(vkResetCommandPool(m_Device, m_Handle, 0));
    }
}