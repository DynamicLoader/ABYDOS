#ifndef __K_MEM_HPP__
#define __K_MEM_HPP__

#include <cstdint>
#include <cstdlib>
#include <malloc.h>

template <typename T> T *alignedMalloc(size_t size, size_t alignment)
{
    return static_cast<T *>(memalign(alignment, size));
}

template <typename T> void alignedFree(T *aligned_ptr)
{
    free(aligned_ptr);
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