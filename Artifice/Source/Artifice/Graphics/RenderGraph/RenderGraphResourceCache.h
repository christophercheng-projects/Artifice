#pragma once

#include <vector>

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Log.h"
#include "Artifice/Core/TemporaryCache.h"

#include "Artifice/Graphics/Device.h"
#include "Artifice/Graphics/Resources.h"


class RenderGraphFramebufferCache : public TemporaryImmutableCache<FramebufferInfo, RenderHandle, 4 + AR_FRAME_COUNT>
{
private:
    Device* m_Device;

public:
    RenderGraphFramebufferCache() = default;
    RenderGraphFramebufferCache(const RenderGraphFramebufferCache& other) = default;
    RenderGraphFramebufferCache(Device* device) : m_Device(device) {}
    void Init(Device* device)
    {
        m_Device = device;
    }

private:
    RenderHandle CreateValue(FramebufferInfo key) override
    {
        return m_Device->CreateFramebuffer(key);
    }
    void DestroyValue(RenderHandle value) override
    {
        m_Device->DestroyFramebuffer(value);
    }
};

class RenderGraphRenderPassCache : public TemporaryImmutableCache<RenderPassInfo, RenderHandle, 4 + AR_FRAME_COUNT>
{
private:
    Device* m_Device;

public:
    RenderGraphRenderPassCache() = default;
    RenderGraphRenderPassCache(const RenderGraphRenderPassCache& other) = default;
    RenderGraphRenderPassCache(Device* device) : m_Device(device) {}
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

class RenderGraphTextureCache : public TemporaryCache<TextureInfo, RenderHandle, 4 + AR_FRAME_COUNT>
{
private:
    Device* m_Device;

public:
    RenderGraphTextureCache() = default;
    RenderGraphTextureCache(const RenderGraphTextureCache& other) = default;
    RenderGraphTextureCache(Device* device) : m_Device(device) {}
    void Init(Device* device)
    {
        m_Device = device;
    }

private:
    RenderHandle CreateValue(TextureInfo key) override
    {
        return m_Device->CreateTexture(key);
    }
    void DestroyValue(RenderHandle value) override
    {
        m_Device->DestroyTexture(value);
    }
};

class RenderGraphBufferCache : public TemporaryCache<BufferInfo, RenderHandle, 4 + AR_FRAME_COUNT>
{
private:
    Device* m_Device;

public:
    RenderGraphBufferCache() = default;
    RenderGraphBufferCache(const RenderGraphBufferCache& other) = default;
    RenderGraphBufferCache(Device* device) : m_Device(device) {}

    void Init(Device* device)
    {
        m_Device = device;
    }

private:
    RenderHandle CreateValue(BufferInfo key) override
    {
        return m_Device->CreateBuffer(key);
    }
    void DestroyValue(RenderHandle value) override
    {
        m_Device->DestroyBuffer(value);
    }
};
