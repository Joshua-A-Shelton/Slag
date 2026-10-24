#pragma once

// Defragmentation passes (ADR 0062): compact movable, automatically placed
// resources into fewer memory blocks.

#include <slag/MemoryBlock.h>

#include <cstdint>
#include <span>

namespace slag {

class Buffer;
class CommandBuffer;
class Texture;

struct DefragmentationDesc
{
    MemoryKind kind = MemoryKind::GpuOnly; // GpuOnly or GpuUpload
    uint64_t maxBytes = 0;                 // bytes copied; 0: no limit
    uint32_t maxMoves = 0;                 // 0: no limit
};

// One moved resource: exactly one of the two is set. After the pass, its
// descriptors and GPU address are stale and must be written again; a moved
// GpuUpload buffer also has a new mappedData().
struct DefragmentationMove
{
    Buffer* buffer = nullptr;
    Texture* texture = nullptr;
};

// From GraphicsDevice::beginDefragmentation until endDefragmentation. Not
// thread-safe; moved resources must not be destroyed while it is active.
class DefragmentationPass
{
public:
    // Empty when nothing can be compacted within the budget.
    std::span<const DefragmentationMove> moves() const;

    // Records every move's copy, outside render passes, on any queue type.
    // Before: moved textures are in CopySource (Common on copy queues), and
    // earlier writes to moved resources are visible to copies. After: the
    // new textures are in that same layout with no access pending.
    void recordCopies(CommandBuffer* commandBuffer);

    DefragmentationPass(const DefragmentationPass&) = delete;
    DefragmentationPass& operator=(const DefragmentationPass&) = delete;

protected:
    DefragmentationPass() = default;
    ~DefragmentationPass() = default;
};

} // namespace slag
