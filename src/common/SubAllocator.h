#pragma once

// Automatic placement (ADRs 0023, 0061): sub-allocates resources from
// internal memory blocks, one pool per memory kind, with TLSF in each block.
// Resources over half a block get dedicated memory instead (the caller's
// job). Thread-safe: one lock per pool (ADR 0068).

#include "common/Objects.h"
#include "common/Tlsf.h"

#include <mutex>

namespace slag {

struct PoolBlock
{
    explicit PoolBlock(const HostAllocator* allocator)
        : tlsf(allocator)
    {}

    MemoryBlockCommon* memoryBlock = nullptr;
    Tlsf tlsf;
};

class SubAllocator
{
public:
    explicit SubAllocator(GraphicsDeviceCommon* device);
    ~SubAllocator(); // releases every block

    SubAllocator(const SubAllocator&) = delete;
    SubAllocator& operator=(const SubAllocator&) = delete;

    uint64_t blockSize(MemoryKind kind) const
    {
        return blockSizes[static_cast<size_t>(kind)];
    }

    // Whether a resource of this size is sub-allocated rather than dedicated.
    bool fits(MemoryKind kind, const PlacementInfo& placement) const
    {
        return placement.size <= blockSize(kind) / 2;
    }

    // Ok with the block and offset to place the resource at.
    Status allocate(MemoryKind kind, const PlacementInfo& placement, bool movable, PoolAllocation& out,
                    MemoryBlockCommon*& memoryBlock, uint64_t& offset);
    // Records the resource that owns an allocation (for defragmentation).
    void setOwner(const PoolAllocation& allocation, void* owner);
    void free(PoolAllocation& allocation);

private:
    GraphicsDeviceCommon* device;
    uint64_t blockSizes[4] = {};
    std::mutex locks[4];
    Vector<PoolBlock*> pools[4];
};

} // namespace slag
