#include "DescriptorSetCache.h"

#include "Device.h"


RenderHandle DescriptorSetCache::CreateValue(UpdatedDescriptorSet key)
{
    RenderHandle ds = m_Device->CreateDescriptorSet(key.Layout);
    m_Device->UpdateDescriptorSet(ds, key.Update);
    
    return ds;
}
void DescriptorSetCache::DestroyValue(RenderHandle value)
{
    m_Device->DestroyDescriptorSet(value);
}