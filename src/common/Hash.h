#pragma once

// A small 64-bit hash for cache keys (ADRs 0043, 0061): FNV-1a with a final
// mix. Collisions are harmless where it is used.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace slag {

class Hasher
{
public:
    void addBytes(const void* data, size_t size)
    {
        auto* bytes = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < size; ++i)
        {
            state = (state ^ bytes[i]) * 0x100000001b3ull;
        }
    }

    // Values without padding: integers, enums, floats, bools.
    template <class T>
    void add(T value)
    {
        static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>);
        addBytes(&value, sizeof(value));
    }

    void addString(const char* text)
    {
        size_t length = text ? std::strlen(text) : 0;
        add(length);
        addBytes(text, length);
    }

    uint64_t finish() const
    {
        uint64_t x = state; // splitmix64 finalizer
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
        return x ^ (x >> 31);
    }

private:
    uint64_t state = 0xcbf29ce484222325ull;
};

} // namespace slag
