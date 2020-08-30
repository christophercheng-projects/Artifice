#pragma once

#include <stdint.h>


typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint8 byte;

#define BIT(x) (1 << x)

#define MIPS(x, y) (uint32) std::floor(std::log2(std::max(x, y))) + 1

#define AR_FRAME_COUNT 2
#define AR_HANDLE_RING_SIZE 8 + AR_FRAME_COUNT

#define PRESENT_QUEUE QueueType::Universal

#define DEFINE_BITMASK_OPERATORS(x, t) \
    static bool Contains(x lhs, x rhs) \
    {                                  \
        return (t)lhs & (t)rhs;        \
    }                                  \
    static x operator~(x rhs)          \
    {                                  \
        t flags = ~(t)rhs;             \
        return (x)flags;               \
    }                                  \
    static x operator|(x lhs, x rhs)   \
    {                                  \
        t flags = (t)lhs | (t)rhs;     \
        return (x)flags;               \
    }                                  \
    static x &operator|=(x &lhs, x rhs)\
    {                                  \
        t flags = (t)lhs | (t)rhs;     \
        lhs = (x)flags;                \
        return lhs;                    \
    }                                  \
    static x operator&(x lhs, x rhs)   \
    {                                  \
        t flags = (t)lhs & (t)rhs;     \
        return (x)flags;               \
    }                                  \
    static x &operator&=(x &lhs, x rhs)\
    {                                  \
        t flags = (t)lhs & (t)rhs;     \
        lhs = (x)flags;                \
        return lhs;                    \
    }                                  \
    static x operator^(x lhs, x rhs)   \
    {                                  \
        t flags = (t)lhs ^ (t)rhs;     \
        return (x)flags;               \
    }                                  \
    static x &operator^=(x &lhs, x rhs)\
    {                                  \
        t flags = (t)lhs ^ (t)rhs;     \
        lhs = (x)flags;                \
        return lhs;                    \
    }                                  
