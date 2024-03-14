#ifndef __K_MEM_H__
#define __K_MEM_H__

#include <cstdint>
#include <cstdlib>

template <typename T> inline T *alignedMalloc(size_t size, int alignment)
{
    const int pointerSize = sizeof(void *);
    const int requestedSize = size + alignment - 1 + pointerSize;
    void *raw = malloc(requestedSize);
    uintptr_t start = (uintptr_t)raw + pointerSize;
    void *aligned = (void *)((start + alignment - 1) & ~(alignment - 1));
    *(void **)((uintptr_t)aligned - pointerSize) = raw;
    return reinterpret_cast<T *>(aligned);
}

template <typename T> void alignedFree(T *aligned_ptr)
{
    if (aligned_ptr)
    {
        free(((T **)aligned_ptr)[-1]);
    }
}

inline bool isAligned(void *data, int alignment)
{
    return ((uintptr_t)data & (alignment - 1)) == 0;
}

constexpr auto addr2page4K(uintptr_t addr)
{
    return addr >> 12; // 4KB page
}

constexpr auto addr2page2M(uintptr_t addr)
{
    return addr >> 21;
}

constexpr auto addr2page1G(uintptr_t addr)
{
    return addr >> 30;
}

#endif