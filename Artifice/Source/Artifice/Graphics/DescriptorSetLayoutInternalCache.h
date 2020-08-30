#pragma once

#include <vector>
#include <set>

#include <vulkan/vulkan.h>

#include "Artifice/Core/Cache.h"

#include "Artifice/Graphics/Resources.h"

#define DESCRIPTOR_POOL_SET_COUNT 16

class DescriptorSetLayoutInternal
{
private:
    VkDevice m_Device;
    VkDescriptorSetLayout m_DescriptorSetLayout;

    std::vector<VkDescriptorPoolSize> m_PoolSizes;

    std::vector<VkDescriptorPool> m_Pools;

    std::vector<VkDescriptorSet> m_FreeDescriptorSets;
    std::set<VkDescriptorSet> m_ActiveDescriptorSets;

public:
    DescriptorSetLayoutInternal() = default;
    DescriptorSetLayoutInternal(VkDevice device, const DescriptorSetLayout& info);

    void CleanUp();

    VkDescriptorSet AllocateDescriptorSet();
    void ReleaseDescriptorSet(VkDescriptorSet descriptor_set);

    VkDescriptorSetLayout GetRaw() const { return m_DescriptorSetLayout; }

private:
    void AllocatePool();
};


class DescriptorSetLayoutInternalCache : public ImmutableCache<DescriptorSetLayout, DescriptorSetLayoutInternal>
{
private:
    VkDevice m_Device;

public:
    DescriptorSetLayoutInternalCache() = default;
    DescriptorSetLayoutInternalCache(VkDevice device);

    void Init(VkDevice device);

private:
    DescriptorSetLayoutInternal CreateValue(DescriptorSetLayout key) override;
    void DestroyValue(DescriptorSetLayoutInternal value) override;

};