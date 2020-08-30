#pragma once

#include "Artifice/Core/Core.h"

#include "Artifice/Core/TemporaryCache.h"
#include "Artifice/Graphics/Resources.h"
#include "Artifice/Graphics/RenderHandle.h"

#if 0
struct UpdatedDescriptorSet
{
    uint32 Set;
    DescriptorSetUpdate Update;

    bool operator==(const UpdatedDescriptorSet& other) const
    {
        return Set == other.Set && Update == other.Update;
    }
    bool operator!=(const UpdatedDescriptorSet& other) const
    {
        return !(*this == other);
    }
    bool operator<(const UpdatedDescriptorSet& other) const
    {
        if (Set != other.Set) 
            return Set < other.Set;
        if (Update != other.Update)
            return Update < other.Update;

        return false;
    }
};

class DescriptorSetCache : public TemporaryImmutableCache<UpdatedDescriptorSet, RenderHandle, 4 + AR_FRAME_COUNT>
{
private:
    Shader* m_Shader;

public:
    DescriptorSetCache() = default;
    DescriptorSetCache(Shader* shader) : m_Shader(shader) {}

    void Init(Shader* shader)
    {
        m_Shader = shader;
    }

private:
    RenderHandle CreateValue(UpdatedDescriptorSet key) override;
    void DestroyValue(RenderHandle value) override;
};
#endif

class Device;

struct UpdatedDescriptorSet
{
    DescriptorSetLayout Layout;
    DescriptorSetUpdate Update;

    bool operator==(const UpdatedDescriptorSet& other) const
    {
        return Layout == other.Layout && Update == other.Update;
    }
    bool operator!=(const UpdatedDescriptorSet& other) const
    {
        return !(*this == other);
    }
    bool operator<(const UpdatedDescriptorSet& other) const
    {
        if (Layout != other.Layout) 
            return Layout < other.Layout;
        if (Update != other.Update)
            return Update < other.Update;

        return false;
    }
};

class DescriptorSetCache : public TemporaryImmutableCache<UpdatedDescriptorSet, RenderHandle, 4 + AR_FRAME_COUNT>
{
private:
    Device* m_Device;

public:
    DescriptorSetCache() = default;
    DescriptorSetCache(Device* device) : m_Device(device) {}

    void Init(Device* device)
    {
        m_Device = device;
    }

private:
    RenderHandle CreateValue(UpdatedDescriptorSet key) override;
    void DestroyValue(RenderHandle value) override;
};