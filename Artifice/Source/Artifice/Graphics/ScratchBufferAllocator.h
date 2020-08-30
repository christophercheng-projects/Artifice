#pragma once

#include <vector>

#include "Artifice/Core/Core.h"

#include "Artifice/Graphics/RenderHandle.h"
#include "Artifice/Graphics/Resources.h"

class Device;


// Scratch buffers are persistently mapped (MemoryAccess:Cpu) buffers for convenient user access
// Linear allocator system with chunks
// Each frame, we reset by setting each chunk's "Allocated" to 0
// TODO: system for deleting buffers after not used...currently system only grows. Or could set a hard cap/limit.

struct ScratchBuffer
{
    RenderHandle Buffer;
    uint64 Offset;
    uint64 Range;
    void* Pointer;
};


class ScratchBufferAllocator
{
private:
    struct Chunk
    {
        RenderHandle Buffer;
        uint64 Size;
        uint64 Allocated;
        void* Pointer;
    };

private:
    Device* m_Device;
    uint64 m_ChunkSize;
    RenderBindFlags m_BindFlags;
    uint32 m_Alignment = 0;

private:
    std::vector<Chunk> m_Chunks;

public:
    ScratchBufferAllocator() = default;
    ScratchBufferAllocator(Device* device, uint64 chunk_size, RenderBindFlags bind_flags);

    void CleanUp();

    ScratchBuffer Allocate(uint64 size, uint32 count = 1);

    void Reset();

private:
    uint64 GetAlignedSize(uint64 size);
    Chunk AllocateChunk(uint64 size);
};