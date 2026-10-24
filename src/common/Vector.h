#pragma once

// Growable array on a HostAllocator (ADR 0066). Operations that may grow return
// false on allocation failure; callers propagate OutOfHostMemory.

#include "common/HostAllocator.h"

#include <cstddef>
#include <new>
#include <utility>

namespace slag {

template <class T>
class Vector
{
public:
    Vector() = default;

    explicit Vector(const HostAllocator* allocator)
        : allocator(allocator)
    {}

    Vector(const Vector&) = delete;
    Vector& operator=(const Vector&) = delete;

    Vector(Vector&& other) noexcept
        : allocator(other.allocator)
        , elements(other.elements)
        , count(other.count)
        , capacityCount(other.capacityCount)
    {
        other.elements = nullptr;
        other.count = 0;
        other.capacityCount = 0;
    }

    Vector& operator=(Vector&& other) noexcept
    {
        if (this != &other)
        {
            release();
            allocator = other.allocator;
            elements = other.elements;
            count = other.count;
            capacityCount = other.capacityCount;
            other.elements = nullptr;
            other.count = 0;
            other.capacityCount = 0;
        }
        return *this;
    }

    ~Vector()
    {
        release();
    }

    [[nodiscard]] bool reserve(size_t newCapacity)
    {
        if (newCapacity <= capacityCount)
        {
            return true;
        }
        T* newElements = static_cast<T*>(hostAllocator().allocate(newCapacity * sizeof(T), alignof(T)));
        if (!newElements)
        {
            return false;
        }
        for (size_t i = 0; i < count; ++i)
        {
            new (&newElements[i]) T(std::move(elements[i]));
            elements[i].~T();
        }
        hostAllocator().free(elements, capacityCount * sizeof(T));
        elements = newElements;
        capacityCount = newCapacity;
        return true;
    }

    [[nodiscard]] bool pushBack(const T& value)
    {
        if (count == capacityCount && !reserve(capacityCount == 0 ? 4 : capacityCount * 2))
        {
            return false;
        }
        new (&elements[count]) T(value);
        ++count;
        return true;
    }

    [[nodiscard]] bool pushBack(T&& value)
    {
        if (count == capacityCount && !reserve(capacityCount == 0 ? 4 : capacityCount * 2))
        {
            return false;
        }
        new (&elements[count]) T(std::move(value));
        ++count;
        return true;
    }

    void popBack()
    {
        --count;
        elements[count].~T();
    }

    // Keeps the capacity, so refilling doesn't allocate (ADR 0066).
    void clear()
    {
        for (size_t i = 0; i < count; ++i)
        {
            elements[i].~T();
        }
        count = 0;
    }

    size_t size() const
    {
        return count;
    }

    size_t capacity() const
    {
        return capacityCount;
    }

    bool empty() const
    {
        return count == 0;
    }

    T* data()
    {
        return elements;
    }

    const T* data() const
    {
        return elements;
    }

    T& operator[](size_t index)
    {
        return elements[index];
    }

    const T& operator[](size_t index) const
    {
        return elements[index];
    }

    T* begin()
    {
        return elements;
    }

    T* end()
    {
        return elements + count;
    }

    const T* begin() const
    {
        return elements;
    }

    const T* end() const
    {
        return elements + count;
    }

private:
    const HostAllocator& hostAllocator() const
    {
        static const HostAllocator defaultAllocator;
        return allocator ? *allocator : defaultAllocator;
    }

    void release()
    {
        clear();
        hostAllocator().free(elements, capacityCount * sizeof(T));
        elements = nullptr;
        capacityCount = 0;
    }

    const HostAllocator* allocator = nullptr;
    T* elements = nullptr;
    size_t count = 0;
    size_t capacityCount = 0;
};

} // namespace slag
