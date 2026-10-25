#pragma once

// Defragmentation passes (ADR 0062): compact movable, automatically placed
// resources into fewer memory blocks.

#include <slag/Barriers.h>
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
// descriptors, GPU address, and native handles (slag/native/) are stale and
// must be fetched or written again; a moved GpuUpload buffer also has a new
// mappedData().
struct DefragmentationMove
{
    Buffer* buffer = nullptr;
    Texture* texture = nullptr;
};

// From GraphicsDevice::beginDefragmentation until endDefragmentation. Not
// thread-safe. While it is active, moved resources are frozen: record
// nothing on them, don't write a moved GpuUpload buffer through its mapped
// pointer (writes after the copy are lost), don't use them from other threads
// (gpuAddress() and mappedData() included), and don't destroy them.
class DefragmentationPass
{
public:
    // Empty when nothing can be compacted within the budget.
    std::span<const DefragmentationMove> moves() const;

    // Records every move's copy, outside render passes, on any queue type.
    // layouts[i] is the layout of moves()[i]'s texture on every subresource
    // (ignored for buffers); Undefined means its contents don't matter, so it
    // isn't copied. The layouts suit the queue: Common on copy queues;
    // Common, ShaderRead, ShaderWrite, CopySource, or CopyDest on compute
    // queues. Nothing earlier in the same submit may use a moved resource:
    // their last uses are in earlier submits, finished and visible
    // (ADR 0049). After endDefragmentation each moved texture is in its layout
    // with no access pending; its next barrier, in a later submit, starts from
    // syncBefore = None (and from Undefined for those passed as Undefined).
    void recordCopies(CommandBuffer* commandBuffer, std::span<const TextureLayout> layouts);

    DefragmentationPass(const DefragmentationPass&) = delete;
    DefragmentationPass& operator=(const DefragmentationPass&) = delete;

protected:
    DefragmentationPass() = default;
    ~DefragmentationPass() = default;
};

} // namespace slag
