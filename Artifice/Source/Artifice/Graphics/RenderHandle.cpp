#include "RenderHandle.h"

#include "RenderHandleAllocator.h"

#if 0

RenderHandle::RenderHandle(const RenderHandle& other)
    : m_ID(other.m_ID), m_Type(other.m_Type), m_AllocatorTag(other.m_AllocatorTag)
{
    if (!IsNull())
    {
        RenderHandleAllocator::Increment(*this);
    }
}

RenderHandle::~RenderHandle()
{
    if (!IsNull())
    {
        RenderHandleAllocator::Decrement(*this);
    }
}

RenderHandle& RenderHandle::operator=(const RenderHandle& other)
{
    if (*this == other) return *this;
    
    if (!IsNull())
    {
        RenderHandleAllocator::Decrement(*this);
    }

    m_ID = other.m_ID;
    m_Type = other.m_Type;
    m_AllocatorTag = other.m_AllocatorTag;

    if (!IsNull())
    {
        RenderHandleAllocator::Increment(*this);
    }
    
    return *this;
}

#endif