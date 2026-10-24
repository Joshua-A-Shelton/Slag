#pragma once

// SLAG_FLAGS(EnumType): bitwise operators for flag enums (CLAUDE.md §10).
// Flag enums must have an unsigned underlying type of at most 64 bits.

#include <cstdint>

#define SLAG_FLAGS(EnumType)                                                                                           \
    constexpr EnumType operator|(EnumType a, EnumType b)                                                               \
    {                                                                                                                  \
        return static_cast<EnumType>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));                             \
    }                                                                                                                  \
    constexpr EnumType operator&(EnumType a, EnumType b)                                                               \
    {                                                                                                                  \
        return static_cast<EnumType>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));                             \
    }                                                                                                                  \
    constexpr EnumType operator^(EnumType a, EnumType b)                                                               \
    {                                                                                                                  \
        return static_cast<EnumType>(static_cast<uint64_t>(a) ^ static_cast<uint64_t>(b));                             \
    }                                                                                                                  \
    constexpr EnumType operator~(EnumType a)                                                                           \
    {                                                                                                                  \
        return static_cast<EnumType>(~static_cast<uint64_t>(a));                                                       \
    }                                                                                                                  \
    constexpr EnumType& operator|=(EnumType& a, EnumType b)                                                            \
    {                                                                                                                  \
        return a = a | b;                                                                                              \
    }                                                                                                                  \
    constexpr EnumType& operator&=(EnumType& a, EnumType b)                                                            \
    {                                                                                                                  \
        return a = a & b;                                                                                              \
    }                                                                                                                  \
    constexpr EnumType& operator^=(EnumType& a, EnumType b)                                                            \
    {                                                                                                                  \
        return a = a ^ b;                                                                                              \
    }
