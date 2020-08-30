#pragma once

#include <vector>

#include "Artifice/Core/Core.h"
#include "Artifice/Core/DataBuffer.h"

#if 0
class ScratchAllocator
{
private:
    struct Chunk
    {
        uint64 Allocated;
        DataBuffer Data;
    };

    uint64 m_ChunkSize = 1024 * 4; // Default 4 kB
    std::vector<Chunk> m_Chunks;

public:
    ScratchAllocator(uint64 chunk_size = 1024 * 4) : m_ChunkSize(chunk_size)
    {

    }
    // Returns pointer to allocation
    byte* Allocate(uint64 size)
    {
        if (size == 0) 
            return nullptr;

        for (uint32 i = 0; i < m_Chunks.size(); i++)
        {
            Chunk& chunk = m_Chunks[i];
            if (chunk.Allocated + size > chunk.Data.GetSize())
            {
                continue;
            }

            byte* result = chunk.Data.Data + chunk.Allocated;

            chunk.Allocated += size;

            return result;
        }

        // No chunk found
        uint64 chunk_size = size > m_ChunkSize ? size : m_ChunkSize; // max(size, m_ChunkSize)
        m_Chunks.push_back({size, DataBuffer()});

        Chunk& chunk = m_Chunks.back();
        chunk.Data.Allocate(chunk_size);

        byte* result = chunk.Data.Data;

        return result;
    }

    template <typename T>
    T* Allocate(uint64 count)
    {
        return (T*)Allocate(sizeof(T) * count);
    }
    template <typename T>
    void Destroy(uint64 count, T* objects)
    {
        for (uint64 i = 0; i < count; i++)
        {
            objects[i].~T();
        }
    }

    void Reset()
    {
        for (uint32 i = 0; i < m_Chunks.size(); i++)
        {
            m_Chunks[i].Allocated = 0;
            m_Chunks[i].Data.ZeroInitialize();
        }
    }
};
#endif
class ScratchAllocator
{
private:
    struct Chunk
    {
        uint64 Allocated;
        DataBuffer Data;
    };

    uint64 m_ChunkSize = 1024 * 4; // Default 4 kB
    std::vector<Chunk> m_Chunks;

public:
    ScratchAllocator(uint64 chunk_size = 1024 * 4) : m_ChunkSize(chunk_size)
    {

    }
    // Returns pointer to allocation
    byte* Allocate(uint64 size)
    {
        if (size == 0) 
            return nullptr;

        for (uint32 i = 0; i < m_Chunks.size(); i++)
        {
            Chunk& chunk = m_Chunks[i];
            if (chunk.Allocated + size > chunk.Data.GetSize())
            {
                continue;
            }

            byte* result = chunk.Data.Data + chunk.Allocated;

            chunk.Allocated += size;

            return result;
        }

        // No chunk found
        uint64 chunk_size = size > m_ChunkSize ? size : m_ChunkSize; // max(size, m_ChunkSize)
        m_Chunks.push_back({size, DataBuffer()});

        Chunk& chunk = m_Chunks.back();
        chunk.Data.Allocate(chunk_size);

        byte* result = chunk.Data.Data;

        return result;
    }

    template <typename T>
    T* Allocate(uint64 count)
    {
        T* ret =  (T*)Allocate(sizeof(T) * count);
        for (uint32 i = 0; i < count; i++)
        {
            new(&ret[i]) T();
        }

        return ret;
    }
    template <typename T>
    void Destroy(uint64 count, T* objects)
    {
        for (uint64 i = 0; i < count; i++)
        {
            objects[i].~T();
        }
    }

    void Reset()
    {
        for (uint32 i = 0; i < m_Chunks.size(); i++)
        {
            m_Chunks[i].Allocated = 0;
        }
    }
};

class ScopeAllocator
{
    std::vector<ScratchAllocator*> m_ScratchAllocators;

public:
    ScopeAllocator() = default;
    ~ScopeAllocator()
    {
        for (uint32 i = 0; i < m_ScratchAllocators.size(); i++)
        {
            delete m_ScratchAllocators[i];
        }
    }

    ScratchAllocator* RequestScope()
    {
        if (m_ScratchAllocators.size())
        {
            ScratchAllocator* ret = m_ScratchAllocators.back();
            m_ScratchAllocators.pop_back();
            return ret;
        }

        return new ScratchAllocator();
    }
    void ReturnScope(ScratchAllocator* allocator)
    {
        allocator->Reset();
        m_ScratchAllocators.push_back(allocator);
    }
};