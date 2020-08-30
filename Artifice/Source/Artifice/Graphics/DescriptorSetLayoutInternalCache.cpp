#include "DescriptorSetLayoutInternalCache.h"

#include "Artifice/Graphics/RenderBackend.h"


DescriptorSetLayoutInternal::DescriptorSetLayoutInternal(VkDevice device, const DescriptorSetLayout& info) : m_Device(device)
{
    // Create descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> bindings(info.Bindings.size());
    for (uint32 i = 0; i < info.Bindings.size(); i++)
    {
        const DescriptorSetLayout::Binding& binding = info.Bindings[i];

        bindings[i].binding = binding.Binding;
        bindings[i].descriptorType = RenderBackend::ConvertDescriptorType(binding.Type);
        bindings[i].descriptorCount = binding.Count;
        bindings[i].stageFlags = RenderBackend::ConvertShaderStage(binding.Stage);
        bindings[i].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo layout_ci = {};
    layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_ci.bindingCount = info.Bindings.size();
    layout_ci.pBindings = bindings.data();

    VK_CALL(vkCreateDescriptorSetLayout(m_Device, &layout_ci, nullptr, &m_DescriptorSetLayout));
    

    // Find pool sizes for allocation
    std::unordered_map<VkDescriptorType, uint32> counts;
    for (uint32 i = 0; i < info.Bindings.size(); i++)
    {
        const DescriptorSetLayout::Binding& binding = info.Bindings[i];
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

void DescriptorSetLayoutInternal::CleanUp()
{
    m_ActiveDescriptorSets.clear();
    m_FreeDescriptorSets.clear();

    for (auto& pool : m_Pools)
    {
        vkResetDescriptorPool(m_Device, pool, 0);
        vkDestroyDescriptorPool(m_Device, pool, nullptr);
    }

    vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
}

VkDescriptorSet DescriptorSetLayoutInternal::AllocateDescriptorSet()
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

void DescriptorSetLayoutInternal::ReleaseDescriptorSet(VkDescriptorSet descriptor_set)
{
    AR_CORE_ASSERT(m_ActiveDescriptorSets.count(descriptor_set), "");

    m_ActiveDescriptorSets.erase(descriptor_set);
    m_FreeDescriptorSets.push_back(descriptor_set);
}

void DescriptorSetLayoutInternal::AllocatePool()
{
    VkDescriptorPool pool;
    VkDescriptorPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.poolSizeCount = m_PoolSizes.size();
    pool_ci.pPoolSizes = m_PoolSizes.data();
    pool_ci.maxSets = DESCRIPTOR_POOL_SET_COUNT;

    VK_CALL(vkCreateDescriptorPool(m_Device, &pool_ci, nullptr, &pool));

    VkDescriptorSet sets[DESCRIPTOR_POOL_SET_COUNT];
    VkDescriptorSetLayout layouts[DESCRIPTOR_POOL_SET_COUNT];
    for (unsigned int i = 0; i < DESCRIPTOR_POOL_SET_COUNT; i++)
    {
        layouts[i] = m_DescriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo descriptor_set_ai = {};
    descriptor_set_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_ai.descriptorPool = pool;
    descriptor_set_ai.descriptorSetCount = DESCRIPTOR_POOL_SET_COUNT;
    descriptor_set_ai.pSetLayouts = layouts;

    VK_CALL(vkAllocateDescriptorSets(m_Device, &descriptor_set_ai, sets));

    for (auto set : sets)
    {
        m_FreeDescriptorSets.push_back(set);
    }

    m_Pools.push_back(pool);
}


DescriptorSetLayoutInternalCache::DescriptorSetLayoutInternalCache(VkDevice device) : m_Device(device) 
{

}

void DescriptorSetLayoutInternalCache::Init(VkDevice device)
{
    m_Device = device;
}

DescriptorSetLayoutInternal DescriptorSetLayoutInternalCache::CreateValue(DescriptorSetLayout key)
{
    return DescriptorSetLayoutInternal(m_Device, key);
}

void DescriptorSetLayoutInternalCache::DestroyValue(DescriptorSetLayoutInternal value)
{
    value.CleanUp();
}