#pragma once

#include <set>
#include <vector>

#include <vulkan/vulkan.h>

#include "Artifice/Core/Core.h"

#define DESCRIPTOR_POOL_SET_COUNT 16

#if 1
class DescriptorSetAllocator
{
public:
    std::vector<VkDescriptorPoolSize> PoolSizes;

private:
    std::vector<VkDescriptorPool> m_Pools;

    std::vector<VkDescriptorSet> m_FreeDescriptorSets;

    std::set<VkDescriptorSet> m_ActiveDescriptorSets;

public:
    DescriptorSetAllocator() {}
    void CleanUp(VkDevice device);
    VkDescriptorSet AllocateDescriptorSet(VkDevice device, VkDescriptorSetLayout layout);
    void ReleaseDescriptorSet(VkDescriptorSet descriptor_set);

private:
    void AllocatePool(VkDevice device, VkDescriptorSetLayout layout);
};

#else

class DescriptorSetAllocator
{
private:
    Device* m_Device;
    DescriptorSetLayout m_DescriptorSetLayout;

    std::vector<VkDescriptorPoolSize> m_PoolSizes;

    std::vector<VkDescriptorPool> m_Pools;

    std::vector<VkDescriptorSet> m_FreeDescriptorSets;

    std::set<VkDescriptorSet> m_ActiveDescriptorSets;

public:
    DescriptorSetAllocator() = default;
    DescriptorSetAllocator(Device* device, const DescriptorSetLayout& layout);
    void CleanUp();
    VkDescriptorSet AllocateDescriptorSet();
    void ReleaseDescriptorSet(VkDescriptorSet descriptor_set);

private:
    void AllocatePool();
};

#endif