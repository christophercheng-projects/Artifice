#include "ScratchBufferAllocator.h"

#include "Device.h"


ScratchBufferAllocator::ScratchBufferAllocator(Device* device, uint64 chunk_size, RenderBindFlags bind_flags) : m_Device(device), m_BindFlags(bind_flags)
{
    if (Contains(bind_flags, RenderBindFlags::UniformBuffer) || Contains(bind_flags, RenderBindFlags::StorageBuffer))
    {
        m_Alignment = 256; // Guaranteed vulkan maximum by spec
    }

    m_ChunkSize = GetAlignedSize(chunk_size);
}

void ScratchBufferAllocator::CleanUp()
{
    for (uint32 i = 0; i < m_Chunks.size(); i++)
    {
        m_Device->DestroyBuffer(m_Chunks[i].Buffer);
    }

    m_Chunks.clear();
}

ScratchBuffer ScratchBufferAllocator::Allocate(uint64 size, uint32 count)
{
    // Determine aligned allocate size
    uint64 aligned_size = GetAlignedSize(size);

    uint64 allocate_size = aligned_size * count;

    // Try to allocate from existing chunk
    for (uint32 i = 0; i < m_Chunks.size(); i++)
    {
        Chunk& chunk = m_Chunks[i];
        if (chunk.Allocated + allocate_size > chunk.Size)
        {
            continue;
        }

        ScratchBuffer result;
        result.Range = allocate_size;
        result.Buffer = chunk.Buffer;
        result.Offset = chunk.Allocated;
        result.Pointer = (uint8*)chunk.Pointer + chunk.Allocated;

        chunk.Allocated += allocate_size;

        return result;
    }

    // Otherwise, allocate a new chunk (a new API buffer)
    uint64 chunk_size = allocate_size > m_ChunkSize ? allocate_size : m_ChunkSize;
    m_Chunks.push_back(AllocateChunk(chunk_size));

    Chunk& chunk = m_Chunks.back();

    ScratchBuffer result;
    result.Range = allocate_size;
    result.Buffer = chunk.Buffer;
    result.Offset = chunk.Allocated;
    result.Pointer = (uint8*)chunk.Pointer + chunk.Allocated;

    chunk.Allocated += allocate_size;

    return result;
}

void ScratchBufferAllocator::Reset()
{
    for (uint32 i = 0; i < m_Chunks.size(); i++)
    {
        m_Chunks[i].Allocated = 0;
    }
}

uint64 ScratchBufferAllocator::GetAlignedSize(uint64 size)
{
    if (m_Alignment == 0)
    {
        return size;
    }
    else
    {
        uint32 remainder = size % m_Alignment;
        if (remainder == 0)
        {
            return size;
        }
        else
        {
            return size + m_Alignment - remainder;
        }
    }
}

ScratchBufferAllocator::Chunk ScratchBufferAllocator::AllocateChunk(uint64 size)
{
    BufferInfo info;
    info.Size = size;
    info.Access = MemoryAccessType::Cpu;
    info.BindFlags = m_BindFlags;

    RenderHandle buffer = m_Device->CreateBuffer(info);

    Chunk chunk;
    chunk.Buffer = buffer;
    chunk.Size = size;
    chunk.Allocated = 0;
    chunk.Pointer = m_Device->MapBuffer(buffer);

    return chunk;
}
