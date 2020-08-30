#include "DescriptorSetAllocator.h"

#include "Artifice/Core/Log.h"

#include "RenderBackend.h"
#include "Resources.h"


#if 1
void DescriptorSetAllocator::CleanUp(VkDevice device)
{
    m_ActiveDescriptorSets.clear();

    m_FreeDescriptorSets.clear();

    for (auto& pool : m_Pools)
    {
        vkResetDescriptorPool(device, pool, 0);
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
}

VkDescriptorSet DescriptorSetAllocator::AllocateDescriptorSet(VkDevice device, VkDescriptorSetLayout layout)
{
    if (m_FreeDescriptorSets.size() == 0)
    {
        AllocatePool(device, layout);
    }

    VkDescriptorSet ds = m_FreeDescriptorSets.back();
    m_FreeDescriptorSets.pop_back();

    m_ActiveDescriptorSets.insert(ds);

    return ds;
}

void DescriptorSetAllocator::ReleaseDescriptorSet(VkDescriptorSet descriptor_set)
{
    AR_CORE_ASSERT(m_ActiveDescriptorSets.count(descriptor_set), "");

    m_ActiveDescriptorSets.erase(descriptor_set);
    m_FreeDescriptorSets.push_back(descriptor_set);
}

void DescriptorSetAllocator::AllocatePool(VkDevice device, VkDescriptorSetLayout layout)
{
    VkDescriptorPool pool;
    VkDescriptorPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.poolSizeCount = PoolSizes.size();
    pool_ci.pPoolSizes = PoolSizes.data();
    pool_ci.maxSets = DESCRIPTOR_POOL_SET_COUNT;

    VK_CALL(vkCreateDescriptorPool(device, &pool_ci, nullptr, &pool));

    VkDescriptorSet sets[DESCRIPTOR_POOL_SET_COUNT];
    VkDescriptorSetLayout layouts[DESCRIPTOR_POOL_SET_COUNT];
    for (unsigned int i = 0; i < DESCRIPTOR_POOL_SET_COUNT; i++)
    {
        layouts[i] = layout;
    }

    VkDescriptorSetAllocateInfo descriptor_set_ai = {};
    descriptor_set_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_ai.descriptorPool = pool;
    descriptor_set_ai.descriptorSetCount = DESCRIPTOR_POOL_SET_COUNT;
    descriptor_set_ai.pSetLayouts = layouts;

    VK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_ai, sets));

    for (auto set : sets)
    {
        m_FreeDescriptorSets.push_back(set);
    }

    m_Pools.push_back(pool);
}

#else

DescriptorSetAllocator::DescriptorSetAllocator(Device* device, const DescriptorSetLayout& layout) : m_Device(device), m_DescriptorSetLayout(layout)
{
    std::unordered_map<VkDescriptorType, uint32> counts;
    for (uint32 i = 0; i < layout.Info.Bindings.size(); i++)
    {
        const DescriptorSetLayout::Binding& binding = layout.Info.Bindings[i];
        VkDescriptorType type = RenderBackend::ConvertDescriptorType(binding.Type);

        auto find = counts.find(type);
        if (find == counts.end())
        {
            counts.emplace(type, binding.Count);
        }
        else
        {
            find->second += binding.Count;
        }
    }
    for (auto pair : counts)
    {
        VkDescriptorPoolSize pool_size = {};
        pool_size.type = pair.first;
        pool_size.descriptorCount = pair.second * DESCRIPTOR_POOL_SET_COUNT;

        m_PoolSizes.push_back(pool_size);
    }
}

void DescriptorSetAllocator::CleanUp()
{
    m_ActiveDescriptorSets.clear();
    m_FreeDescriptorSets.clear();

    for (auto& pool : m_Pools)
    {
        vkResetDescriptorPool(m_Device->GetHandle(), pool, 0);
        vkDestroyDescriptorPool(m_Device->GetHandle(), pool, nullptr);
    }
}

VkDescriptorSet DescriptorSetAllocator::AllocateDescriptorSet()
{
    if (m_FreeDescriptorSets.size() == 0)
    {
        AllocatePool();
    }

    VkDescriptorSet ds = m_FreeDescriptorSets.back();
    m_FreeDescriptorSets.pop_back();

    m_ActiveDescriptorSets.insert(ds);

    return ds;
}

void DescriptorSetAllocator::ReleaseDescriptorSet(VkDescriptorSet descriptor_set)
{
    AR_CORE_ASSERT(m_ActiveDescriptorSets.count(descriptor_set), "");

    m_ActiveDescriptorSets.erase(descriptor_set);
    m_FreeDescriptorSets.push_back(descriptor_set);
}

void DescriptorSetAllocator::AllocatePool()
{
    VkDescriptorPool pool;
    VkDescriptorPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.poolSizeCount = m_PoolSizes.size();
    pool_ci.pPoolSizes = m_PoolSizes.data();
    pool_ci.maxSets = DESCRIPTOR_POOL_SET_COUNT;

    VK_CALL(vkCreateDescriptorPool(m_Device->GetHandle(), &pool_ci, nullptr, &pool));

    VkDescriptorSet sets[DESCRIPTOR_POOL_SET_COUNT];
    VkDescriptorSetLayout layouts[DESCRIPTOR_POOL_SET_COUNT];
    for (unsigned int i = 0; i < DESCRIPTOR_POOL_SET_COUNT; i++)
    {
        layouts[i] = m_DescriptorSetLayout.DescriptorSetLayoutRaw;
    }

    VkDescriptorSetAllocateInfo descriptor_set_ai = {};
    descriptor_set_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_ai.descriptorPool = pool;
    descriptor_set_ai.descriptorSetCount = DESCRIPTOR_POOL_SET_COUNT;
    descriptor_set_ai.pSetLayouts = layouts;

    VK_CALL(vkAllocateDescriptorSets(m_Device->GetHandle(), &descriptor_set_ai, sets));

    for (auto set : sets)
    {
        m_FreeDescriptorSets.push_back(set);
    }

    m_Pools.push_back(pool);
}

#endif