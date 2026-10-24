#pragma once

// Buffer and BufferDesc (ADRs 0020, 0024, 0025).

#include <slag/MemoryBlock.h>

#include <cstdint>

namespace slag {

struct BufferDesc
{
    uint64_t size = 0;
    Placement placement;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

class Buffer
{
public:
    // 64-bit GPU address (ADR 0025). Thread-safe.
    uint64_t gpuAddress() const;

    // CPU pointer for Upload, Readback, and GpuUpload memory; null otherwise.
    // Mapped for the buffer's lifetime (ADR 0024). Thread-safe.
    void* mappedData() const;

    uint64_t userData() const;
    void setUserData(uint64_t value);

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

protected:
    Buffer() = default;
    ~Buffer() = default;
};

} // namespace slag
