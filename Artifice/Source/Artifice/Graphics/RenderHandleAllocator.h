#pragma once

#include <deque>
#include <unordered_map>
#include <unordered_set>

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Log.h"

#include "RenderHandle.h"

#if 0
#define MAX_RENDER_HANDLE_ALLOCATORS 1

class RenderHandleAllocator
{
    friend class RenderHandle;

private:
    static RenderHandleAllocator s_Allocators[MAX_RENDER_HANDLE_ALLOCATORS + 1];
    static uint8 s_NextTag;

public:
    static RenderHandleAllocator* Create()
    {
        AR_CORE_ASSERT(s_NextTag <= MAX_RENDER_HANDLE_ALLOCATORS + 1, "");
        uint8 tag = s_NextTag++;
        s_Allocators[tag] = RenderHandleAllocator(tag);

        return &s_Allocators[tag];
    }

private:
    static void Increment(const RenderHandle& handle)
    {
        RenderHandleAllocator* allocator = &s_Allocators[handle.GetTag()];
        allocator->m_ActiveIDs[(uint8)handle.GetType()][handle.GetID()]++;
    }
    static void Decrement(const RenderHandle& handle)
    {        
        RenderHandleAllocator* allocator = &s_Allocators[handle.GetTag()];

        uint8 type_index = (uint8)handle.GetType();
        if (--allocator->m_ActiveIDs[type_index][handle.GetID()] == 0)
        {
            allocator->m_ActiveIDs[type_index].erase(handle.GetID());
            allocator->m_InvalidIDs[type_index].erase(handle.GetID());
            allocator->m_FreeIDs[type_index].emplace_back(handle.GetID());
        }
    }

private:
    // ID : Count
    std::unordered_map<uint16, uint32> m_ActiveIDs[(uint8)RenderHandleType::Count];
    std::unordered_set<uint16> m_InvalidIDs[(uint8)RenderHandleType::Count];
    std::deque<uint16> m_FreeIDs[(uint8)RenderHandleType::Count];

    uint16 m_NextIDs[(uint8)RenderHandleType::Count] = {};

    uint8 m_Tag = 0;

private:
    RenderHandleAllocator(uint8 tag) : m_Tag(tag) {}

public:
    RenderHandleAllocator() {}

    RenderHandle Allocate(RenderHandleType type)
    {
        AR_CORE_ASSERT(m_Tag != 0, "");
        AR_CORE_ASSERT(type != RenderHandleType::Count, "");

        uint8 type_index = (uint8)type;

        uint16 id;
        if (m_FreeIDs[type_index].size())
        {
            id = m_FreeIDs[type_index].front();
            m_FreeIDs[type_index].pop_front();
        }
        else
        {
            id = m_NextIDs[type_index]++;
        }
        m_ActiveIDs[type_index].emplace(id, 1);

        RenderHandle handle;
        handle.SetType(type);
        handle.SetID(id);
        handle.SetTag(m_Tag);

        return handle;
    }

    void Release(const RenderHandle& handle)
    {
        AR_CORE_ASSERT(handle.GetTag() != 0, "");
        AR_CORE_ASSERT(IsValid(handle), "");

        m_InvalidIDs[(uint8)handle.GetType()].insert(handle.GetID());
    }

    bool IsValid(const RenderHandle& handle)
    {
        AR_CORE_ASSERT(handle.GetTag() == m_Tag, "");

        if (m_InvalidIDs[(uint8)handle.GetType()].count(handle.GetID()))
            return false;

        return true;
    }
};
#endif
