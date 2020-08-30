#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>

#include <vulkan/vulkan.h>

#include "Artifice/Core/Core.h"
#include "Artifice/Debug/Instrumentor.h"
#include "Artifice/Core/Log.h"
#include "Artifice/Utils/Hash.h"


enum class RenderHandleType : uint8
{
    Swapchain,
    Buffer,
    Texture,
    Sampler,
    ShaderModule,
    PipelineLayout,
    RenderPass,
    GraphicsPipeline,
    ComputePipeline,
    Framebuffer,
    DescriptorSet,

    Count
};
#if 0
class RenderHandle
{
    friend class RenderHandleAllocator;
    friend class RenderHandleStorage;

private:
    uint16 m_ID = 0;
    uint8 m_Type = 0;
    uint8 m_AllocatorTag = 0;

public:
    RenderHandle() = default;

    RenderHandle(const RenderHandle& other);

    ~RenderHandle();

    RenderHandle& operator=(const RenderHandle& other);

    bool operator==(const RenderHandle& other) const
    {
        return GetCookie() == other.GetCookie();
    }
    bool operator!=(const RenderHandle& other) const
    {
        return !(*this == other);
    }
    bool operator<(const RenderHandle& other) const
    {
        return GetCookie() < other.GetCookie();
    }

    bool IsNull() const { return GetCookie() == 0; }

    uint32 GetCookie() const { return m_AllocatorTag << 24 | m_Type << 16 | m_ID; }

public:
    uint16 GetID() const { return m_ID; }
    RenderHandleType GetType() const { return (RenderHandleType)m_Type; }
    uint8 GetTag() const { return m_AllocatorTag; }

private:
    void SetID(uint16 id) { m_ID = id; }
    void SetType(RenderHandleType type) { m_Type = (uint8)type; }
    void SetTag(uint8 tag) { m_AllocatorTag = tag; }
};

namespace std
{
    template <>
    struct hash<RenderHandle>
    {
        size_t operator()(const RenderHandle& k) const
        {
            return k.GetCookie();
        }
    };
} // namespace std

#endif

class RenderHandle
{
    template <uint32 RING_SIZE>
    friend class RenderHandleAllocator;

    friend class RenderHandleStorage;

private:
    uint32 m_ID = 0;
    uint8 m_Type = 0;
    
    uint8 m_Padding[3];

public:
    static RenderHandle Null() { return RenderHandle(); }
public:
    RenderHandle() = default;

    RenderHandle(const RenderHandle& other)
        : m_ID(other.m_ID), m_Type(other.m_Type)
    {
        
    }

    RenderHandle& operator=(const RenderHandle& other)
    {
        m_ID = other.m_ID;
        m_Type = other.m_Type;
        
        return *this;
    }

    bool operator==(const RenderHandle& other) const
    {
        return GetCookie() == other.GetCookie();
    }
    bool operator!=(const RenderHandle& other) const
    {
        return !(*this == other);
    }
    bool operator<(const RenderHandle& other) const
    {
        return GetCookie() < other.GetCookie();
    }

    bool IsNull() const { return GetCookie() == 0; }

    uint64 GetCookie() const { return (uint64)m_Type << 32 | (uint64)m_ID; }

public:
    uint32 GetID() const { return m_ID; }
    RenderHandleType GetType() const { return (RenderHandleType)m_Type; }
    void SetNull() { m_ID = 0; m_Type = 0; }

private:
    void SetID(uint32 id) { m_ID = id; }
    void SetType(RenderHandleType type) { m_Type = (uint8)type; }
};

namespace std
{
    template <>
    struct hash<RenderHandle>
    {
        size_t operator()(const RenderHandle& k) const
        {
            return k.GetCookie();
        }
    };
} // namespace std




template <RenderHandleType TYPE, typename RESOURCE>
class RenderHandleResourceStorage
{
private:
    std::unordered_map<RenderHandle, RESOURCE> m_Resources;
public:
    void Add(RenderHandle handle, RESOURCE resource)
    {
        AR_CORE_ASSERT(handle.GetType() == TYPE, "")

        m_Resources.insert({handle, resource});
    }

    RESOURCE* Get(RenderHandle handle)
    {
        AR_CORE_ASSERT(handle.GetType() == TYPE, "")
        AR_CORE_ASSERT(m_Resources.count(handle), "")

        return &m_Resources[handle];
    }

    void Remove(RenderHandle handle)
    {
        AR_CORE_ASSERT(handle.GetType() == TYPE, "")
        AR_CORE_ASSERT(m_Resources.count(handle), "")

        m_Resources.erase(handle);
    }

};

template <uint32 RING_SIZE>
class RenderHandleAllocator
{
    friend class RenderHandle;

private:
    const uint32 RingBackIndex = RING_SIZE - 1;

    // ID : Count
    std::unordered_set<uint32> m_ActiveIDs[(uint8)RenderHandleType::Count];
    std::vector<uint32> m_InvalidIDs[(uint8)RenderHandleType::Count][RING_SIZE];
    std::vector<uint32> m_FreeIDs[(uint8)RenderHandleType::Count];

    uint16 m_NextIDs[(uint8)RenderHandleType::Count] = {};

public:
    RenderHandleAllocator() = default;

    void Advance()
    {
        AR_PROFILE_FUNCTION();
        
        for (uint8 i = 0; i < (uint8)RenderHandleType::Count; i++)
        {
            // TODO
            // Using an index is definitely better, so don't need to rotate rings.

            // Recycle invalid ids from back ring to free list
            m_FreeIDs[i].insert(m_FreeIDs[i].end(), m_InvalidIDs[i][RingBackIndex].begin(), m_InvalidIDs[i][RingBackIndex].end());

            // Rotate the rings
            for (uint32 j = RingBackIndex; j > 0; j--)
            {
                m_InvalidIDs[i][j] = m_InvalidIDs[i][j - 1];
            }

            // First ring now clear (old back ring)
            m_InvalidIDs[i][0].clear();
        }
    }

    RenderHandle Allocate(RenderHandleType type)
    {
        AR_PROFILE_FUNCTION();
        
        AR_CORE_ASSERT(type != RenderHandleType::Count, "");

        uint8 type_index = (uint8)type;

        uint32 id;
        if (m_FreeIDs[type_index].size())
        {
            id = m_FreeIDs[type_index].back();
            m_FreeIDs[type_index].pop_back();
        }
        else
        {
            id = m_NextIDs[type_index]++;
        }
        m_ActiveIDs[type_index].insert(id);

        RenderHandle handle;
        handle.SetType(type);
        handle.SetID(id);

        return handle;
    }

    void Release(const RenderHandle& handle)
    {
        AR_PROFILE_FUNCTION();
        
        AR_CORE_ASSERT(IsValid(handle), "");

        m_ActiveIDs[(uint8)handle.GetType()].erase(handle.GetID());
        m_InvalidIDs[(uint8)handle.GetType()][0].push_back(handle.GetID());
    }

    bool IsValid(const RenderHandle& handle)
    {
        return m_ActiveIDs[(uint8)handle.GetType()].find(handle.GetID()) != m_ActiveIDs[(uint8)handle.GetType()].end();
    }
};
