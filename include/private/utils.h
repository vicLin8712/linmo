#pragma once

#include <lib/libc.h>

#define unlikely(x) __builtin_expect(!!(x), 0)
#define likely(x) __builtin_expect(!!(x), 1)

#define UNUSED __attribute__((unused))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

/* Align pointer forward to the next 4-byte boundary */
#define ALIGN4(x) ((((uint32_t) (x) + 3u) >> 2) << 2)

static inline bool ispowerof2(uint32_t x)
{
    return x && !(x & (x - 1));
}

static inline uint32_t nextpowerof2(uint32_t x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;

    return x;
}
