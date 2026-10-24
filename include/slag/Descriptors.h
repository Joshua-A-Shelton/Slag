#pragma once

// Descriptor structs for resource and sampler heap writes (ADRs 0030, 0031, 0033).

#include <cstdint>

namespace slag {

class Buffer;

// StructuredBuffer<T> / RWStructuredBuffer<T>. offset must be a multiple of
// stride; count * stride bytes must fit the buffer.
struct StructuredBufferDescriptor
{
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
    uint32_t count = 0;
    uint32_t stride = 0;
};

} // namespace slag
