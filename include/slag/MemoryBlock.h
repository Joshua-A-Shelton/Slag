#pragma once

// MemoryBlock, MemoryBlockDesc, Placement, PlacementInfo (ADRs 0019, 0023,
// 0062).

#include <cstdint>

namespace slag {

enum class MemoryKind : uint8_t
{
    GpuOnly,   // GPU memory, not CPU-visible
    Upload,    // CPU writes, GPU reads (write-combined: write only, sequentially)
    Readback,  // GPU writes, CPU reads (cached)
    GpuUpload, // GPU memory the CPU can write (resizable BAR); optional capability
};

// Memory that placed resources live in (ADR 0023). Any resource type may be
// placed in a GpuOnly block; the CPU-visible kinds hold buffers only.
struct MemoryBlockDesc
{
    uint64_t size = 0;
    MemoryKind kind = MemoryKind::GpuOnly;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

class MemoryBlock
{
public:
    uint64_t size() const;
    MemoryKind kind() const;

    uint64_t userData() const;
    void setUserData(uint64_t value);

    MemoryBlock(const MemoryBlock&) = delete;
    MemoryBlock& operator=(const MemoryBlock&) = delete;

protected:
    MemoryBlock() = default;
    ~MemoryBlock() = default;
};

// Size and alignment a resource needs in a memory block (ADR 0023). Results
// differ between backends and devices.
struct PlacementInfo
{
    uint64_t size = 0;
    uint64_t alignment = 0;
};

struct PlacementOptions
{
    // May be moved by a defragmentation pass (ADR 0062).
    bool movable = false;
};

// Where a resource's memory comes from (ADR 0023): automatic placement in a
// memory kind, or a user MemoryBlock at an offset.
struct Placement
{
    static Placement automatic(MemoryKind kind, PlacementOptions options = {})
    {
        Placement placement;
        placement.kind = kind;
        placement.movable = options.movable;
        return placement;
    }

    static Placement placed(MemoryBlock* memoryBlock, uint64_t offset)
    {
        Placement placement;
        placement.memoryBlock = memoryBlock;
        placement.offset = offset;
        return placement;
    }

    MemoryKind kind = MemoryKind::GpuOnly;
    bool movable = false;
    MemoryBlock* memoryBlock = nullptr; // non-null: placed
    uint64_t offset = 0;
};

} // namespace slag
