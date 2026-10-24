#pragma once

// Wraps AllocatorCallbacks (ADR 0066). Every CPU allocation in Slag goes
// through one of these; a null result is reported as OutOfHostMemory.

#include <slag/Instance.h>

#include <cstddef>
#include <new>
#include <utility>

namespace slag {

class HostAllocator
{
public:
    HostAllocator() = default;
    explicit HostAllocator(const AllocatorCallbacks& callbacks);

    void* allocate(size_t size, size_t alignment) const;
    void free(void* pointer, size_t size) const;

    template <class T, class... Args>
    T* create(Args&&... args) const
    {
        void* memory = allocate(sizeof(T), alignof(T));
        if (!memory)
        {
            return nullptr;
        }
        return new (memory) T(std::forward<Args>(args)...);
    }

    // T must be the object's exact type, so the size given back matches.
    template <class T>
    void destroy(T* object) const
    {
        if (!object)
        {
            return;
        }
        object->~T();
        free(object, sizeof(T));
    }

private:
    AllocatorCallbacks callbacks;
};

} // namespace slag
