#pragma once

// MemoryBlock, MemoryBlockDesc, Placement (ADRs 0019, 0023, 0062).

#include <cstdint>

namespace slag {

class MemoryBlock;

enum class MemoryKind : uint8_t
{
    GpuOnly,   // GPU memory, not CPU-visible
    Upload,    // CPU writes, GPU reads (write-combined: write only, sequentially)
    Readback,  // GPU writes, CPU reads (cached)
    GpuUpload, // GPU memory the CPU can write (resizable BAR); optional capability
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
