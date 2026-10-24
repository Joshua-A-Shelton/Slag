#pragma once

// Two-level segregated fit allocator over one memory block's offsets (ADR
// 0061): constant-time allocate and free, coalescing on free. Keeps owner,
// offset, size, and alignment of every allocation for defragmentation (ADR
// 0062). Not thread-safe; the sub-allocator locks around it.

#include "common/HostAllocator.h"
#include "common/Vector.h"

#include <slag/Result.h>

#include <cstdint>

namespace slag {

class Tlsf
{
public:
    static constexpr uint32_t invalid = UINT32_MAX;
    // Offsets and sizes are multiples of this.
    static constexpr uint64_t granularity = 256;

    struct Node
    {
        uint64_t offset = 0;
        uint64_t size = 0;
        uint32_t prevPhysical = invalid;
        uint32_t nextPhysical = invalid;
        uint32_t prevFree = invalid;
        uint32_t nextFree = invalid;
        bool isFree = false;
        bool unused = false; // slot in the node array is available
        // Bookkeeping for allocations.
        bool movable = false;
        uint64_t alignment = 0;
        void* owner = nullptr;
        bool ownerIsTexture = false; // else a buffer
    };

    explicit Tlsf(const HostAllocator* allocator);

    Tlsf(const Tlsf&) = delete;
    Tlsf& operator=(const Tlsf&) = delete;

    // Manages [0, size). Returns false when out of host memory.
    [[nodiscard]] bool init(uint64_t size);

    // Ok with the node index and offset; Unsupported when nothing fits;
    // OutOfHostMemory. `alignment` is a power of two.
    Status allocate(uint64_t size, uint64_t alignment, void* owner, bool movable, uint32_t& node, uint64_t& offset);
    void free(uint32_t node);
    void setOwner(uint32_t node, void* owner, bool isTexture)
    {
        nodeArray[node].owner = owner;
        nodeArray[node].ownerIsTexture = isTexture;
    }

    bool empty() const
    {
        return allocationCount == 0;
    }
    uint64_t freeBytes() const
    {
        return freeSize;
    }
    const Vector<Node>& nodes() const
    {
        return nodeArray;
    }

private:
    static constexpr uint32_t secondLevelBits = 5;
    static constexpr uint32_t secondLevelCount = 1u << secondLevelBits;
    static constexpr uint32_t firstLevelCount = 64;

    static void mapping(uint64_t size, uint32_t& firstLevel, uint32_t& secondLevel);
    uint32_t findFree(uint64_t size) const;
    void insertFree(uint32_t node);
    void removeFree(uint32_t node);
    [[nodiscard]] bool reserveNodes(size_t count);
    void newNode(uint32_t& node);
    void releaseNode(uint32_t node);

    Vector<Node> nodeArray;
    Vector<uint32_t> unusedNodes;
    uint64_t firstLevelMap = 0;
    uint32_t secondLevelMap[firstLevelCount] = {};
    uint32_t heads[firstLevelCount][secondLevelCount];
    uint64_t freeSize = 0;
    uint32_t allocationCount = 0;
};

} // namespace slag
