#include "common/HostAllocator.h"

#include <cstdlib>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace slag {

namespace {

void* defaultAllocate(size_t size, size_t alignment, void*)
{
    if (alignment < alignof(std::max_align_t))
    {
        alignment = alignof(std::max_align_t);
    }
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    // aligned_alloc requires the size to be a multiple of the alignment.
    size_t rounded = (size + alignment - 1) / alignment * alignment;
    return std::aligned_alloc(alignment, rounded);
#endif
}

void defaultFree(void* pointer, size_t, void*)
{
#ifdef _WIN32
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

} // namespace

HostAllocator::HostAllocator(const AllocatorCallbacks& userCallbacks)
{
    if (userCallbacks.allocate && userCallbacks.free)
    {
        callbacks = userCallbacks;
    }
}

void* HostAllocator::allocate(size_t size, size_t alignment) const
{
    if (callbacks.allocate)
    {
        return callbacks.allocate(size, alignment, callbacks.userData);
    }
    return defaultAllocate(size, alignment, nullptr);
}

void HostAllocator::free(void* pointer, size_t size) const
{
    if (!pointer)
    {
        return;
    }
    if (callbacks.free)
    {
        callbacks.free(pointer, size, callbacks.userData);
        return;
    }
    defaultFree(pointer, size, nullptr);
}

} // namespace slag
