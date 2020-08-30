#pragma once

#include <string>

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Log.h"

struct DataBuffer
{
    byte* Data = nullptr;
    uint64 Size = 0;

    DataBuffer() = default;

    DataBuffer(void* data, uint64 size)
    {
        Allocate(size);
        Write(data, size);
    }

    DataBuffer(const DataBuffer& buffer)
    {
        Allocate(buffer.Size);
        Write(buffer.Data, Size);
    }

    DataBuffer(DataBuffer&& buffer) noexcept
    {
        Data = buffer.Data;
        Size = buffer.Size;

        buffer.Data = nullptr;
        buffer.Size = 0;
    }

    ~DataBuffer()
    {
        if (Data)
            delete[] Data;
    }

    void Allocate(uint64 size)
    {
        delete[] Data;
        Data = nullptr;

        if (size == 0)
            return;

        Data = new byte[size];
        Size = size;
    }

    void Clear()
    {
        if (Data)
            delete[] Data;
        Data = nullptr;
        Size = 0;
    }

    void ZeroInitialize()
    {
        if (Data)
            memset(Data, 0, Size);
    }

    void Read(void* dest, uint64 size, uint32 offset = 0) const
    {
        AR_CORE_ASSERT(offset + size <= Size, "Buffer overflow!");
        memcpy(dest, Data + offset, size);
    }

    void Write(void* data, uint64 size, uint32 offset = 0)
    {
        AR_CORE_ASSERT(offset + size <= Size, "Buffer overflow!");
        memcpy(Data + offset, data, size);
    }

    uint64 GetSize() const { return Size; }

    DataBuffer& operator=(const DataBuffer& other)
    {
        Allocate(other.Size);
        Write(other.Data, Size);

        return *this;
    }

    DataBuffer& operator=(DataBuffer&& other) noexcept
    {
        Data = other.Data;
        Size = other.Size;

        other.Data = nullptr;
        other.Size = 0;

        return *this;
    }

    operator bool() const
    {
        return Data;
    }

    byte& operator[](int index)
    {
        return Data[index];
    }

    byte operator[](int index) const
    {
        return Data[index];
    }

    template <typename T>
    T* As()
    {
        return (T*)Data;
    }
};