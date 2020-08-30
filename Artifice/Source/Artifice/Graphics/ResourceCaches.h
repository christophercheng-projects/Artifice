#pragma once

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Cache.h"
#include "Artifice/Core/TemporaryCache.h"

#include "Artifice/Graphics/Device.h"
#include "Artifice/Graphics/Resources.h"
#include "Artifice/Graphics/RenderHandle.h"


class RenderPassCache : public ImmutableCache<RenderPassInfo, RenderHandle>
{
private:
    Device* m_Device;

public:
    RenderPassCache() = default;
    RenderPassCache(Device* device) : m_Device(device) {}

    void Init(Device* device)
    {
        m_Device = device;
    }

private:
    RenderHandle CreateValue(RenderPassInfo key) override
    {
        return m_Device->CreateRenderPass(key);
    }
    void DestroyValue(RenderHandle value) override
    {
       m_Device->DestroyRenderPass(value);
    }
};

class SamplerCache : public ImmutableCache<SamplerInfo, RenderHandle>
{
private:
    Device* m_Device;

public:
    SamplerCache() = default;
    SamplerCache(Device* device) : m_Device(device) {}

    void Init(Device* device)
    {
        m_Device = device;
    }

private:
    RenderHandle CreateValue(SamplerInfo key) override
    {
        return m_Device->CreateSampler(key);
    }
    void DestroyValue(RenderHandle value) override
    {
       m_Device->DestroySampler(value);
    }
};