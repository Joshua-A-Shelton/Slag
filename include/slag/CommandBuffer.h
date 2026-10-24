#pragma once

// CommandBuffer, RenderingDesc, draw/copy argument structs: recording, render
// passes, binding, push constants and the constant buffer slot, draws, copies,
// barriers, queries, labels (ADRs 0003, 0038, 0044, 0045-0051).

#include <slag/Barriers.h>
#include <slag/CommandQueue.h>
#include <slag/Result.h>

#include <cstdint>
#include <span>

namespace slag {

class Buffer;
class Pipeline;

struct CommandBufferDesc
{
    CommandQueueType queueType = CommandQueueType::Graphics;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

// Owns its allocator; the user resets it once the GPU is done (ADR 0045).
// One thread at a time.
class CommandBuffer
{
public:
    Result<void> begin();
    Result<void> end();
    Result<void> reset();

    void bindPipeline(Pipeline* pipeline);
    void pushConstants(const void* data, uint32_t size, uint32_t offset = 0);

    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    void barrier(std::span<const GlobalBarrier> globalBarriers, std::span<const BufferBarrier> bufferBarriers = {},
                 std::span<const TextureBarrier> textureBarriers = {});

    void copyBuffer(Buffer* destination, uint64_t destinationOffset, Buffer* source, uint64_t sourceOffset,
                    uint64_t size);

    uint64_t userData() const;
    void setUserData(uint64_t value);

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

protected:
    CommandBuffer() = default;
    ~CommandBuffer() = default;
};

} // namespace slag
