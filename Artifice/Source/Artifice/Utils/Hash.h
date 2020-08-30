#pragma once

#include <stdint.h>
#include <stddef.h>

#include "Artifice/Core/Core.h"

// FNV-1 Hash (64 bit)

#define FNV_PRIME_64 0x100000001b3ull
#define FNV_OFFSET_BASIS_64 0xcbf29ce484222325ull

class Hasher
{
private:
    uint64 m_Hash = FNV_OFFSET_BASIS_64;
public:
    Hasher() {}
    Hasher(uint64 hash) : m_Hash(hash) { }

    template <typename T>
    inline void data(const T* data, size_t count)
    {
        size_t byte_count = count * sizeof(T);
        for (size_t i = 0; i < byte_count; i++)
        {
            m_Hash = (m_Hash * FNV_PRIME_64) ^ data[i];
        }
    }

    inline void u8(uint8 value)
    {
        m_Hash = (m_Hash * FNV_PRIME_64) ^ value;
    }

    inline void u32(uint32 value)
    {
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[0];
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[1];
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[2];
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[3];
    }

    inline void u64(uint64 value)
    {
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[0];
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[1];
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[2];
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[3];
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[4];
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[5];
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[6];
        m_Hash = (m_Hash * FNV_PRIME_64) ^ ((uint8*)(&value))[7];
    }

    inline void f32(float value)
    {
        union
		{
			float f32;
			uint32 u32;
		} u;
		u.f32 = value;
		u32(u.u32);
    }

    inline uint64 GetHash() const { return m_Hash; }
};