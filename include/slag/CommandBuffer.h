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
class Texture;

// One mip and layer of a texture, and a texel offset in it (ADR 0048). Depth
// and stencil are copied separately: textures with both name a plane.
struct TextureLocation
{
    Texture* texture = nullptr;
    uint32_t mip = 0;
    uint32_t layer = 0; // array layer or cube face; 0 for 3D textures
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;
    TexturePlane plane = TexturePlane::Color;
};

// Texel data in a buffer, as GraphicsDevice::copyFootprint lays it out: rows
// of blocks rowPitch apart, depth slices rowPitch * rows apart. The offset is a
// multiple of alignment::textureCopyOffset (512), the row pitch of
// alignment::textureCopyRowPitch (256) (ADR 0048).
struct BufferLocation
{
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
    uint32_t rowPitch = 0;
};

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
    // Textures are in the CopySource / CopyDest layouts (Common on copy
    // queues). Sizes are in texels; block-compressed regions cover whole 4x4
    // blocks. Same or castable formats; no MSAA.
    void copyTexture(const TextureLocation& destination, const TextureLocation& source, uint32_t width, uint32_t height,
                     uint32_t depth = 1);
    void copyBufferToTexture(const TextureLocation& destination, const BufferLocation& source, uint32_t width,
                             uint32_t height, uint32_t depth = 1);
    void copyTextureToBuffer(const BufferLocation& destination, const TextureLocation& source, uint32_t width,
                             uint32_t height, uint32_t depth = 1);

    uint64_t userData() const;
    void setUserData(uint64_t value);

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

protected:
    CommandBuffer() = default;
    ~CommandBuffer() = default;
};

} // namespace slag
