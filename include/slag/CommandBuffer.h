#pragma once

// CommandBuffer, RenderingDesc, draw/copy argument structs: recording, render
// passes, binding, push constants and the constant buffer slot, draws, copies,
// barriers, queries, labels (ADRs 0003, 0038, 0044, 0045-0051).

#include <slag/Barriers.h>
#include <slag/CommandQueue.h>
#include <slag/Result.h>
#include <slag/Types.h>

#include <cstdint>
#include <span>

namespace slag {

class AttachmentView;
class Buffer;
class Pipeline;
class Texture;

// --- Render passes (ADR 0046) -------------------------------------------------

enum class LoadOp : uint8_t
{
    Load,
    Clear,
    Discard,
};

enum class StoreOp : uint8_t
{
    Store,
    Discard,
    Resolve, // average into resolveView, discarding this attachment's samples
};

// In the RenderTarget layout when the pass begins.
struct ColorAttachment
{
    AttachmentView* view = nullptr;
    LoadOp loadOp = LoadOp::Load;
    StoreOp storeOp = StoreOp::Store;
    ClearValue clearValue;
    AttachmentView* resolveView = nullptr; // StoreOp::Resolve: single-sampled, same format, RenderTarget layout
};

// In DepthStencilWrite, or DepthStencilRead for read-only planes.
struct DepthStencilAttachment
{
    AttachmentView* view = nullptr;
    LoadOp depthLoadOp = LoadOp::Load;
    StoreOp depthStoreOp = StoreOp::Store;
    LoadOp stencilLoadOp = LoadOp::Load; // ignored without a stencil plane
    StoreOp stencilStoreOp = StoreOp::Store;
    ClearValue clearValue;
    bool readOnlyDepth = false;
    bool readOnlyStencil = false;
};

// Always the full attachments; use scissors to restrict. A pass split across
// command buffers suspends in one and resumes in the next, submitted
// consecutively in one submit.
struct RenderingDesc
{
    std::span<const ColorAttachment> colorAttachments; // up to 8; null views leave gaps
    const DepthStencilAttachment* depthStencilAttachment = nullptr;
    bool suspending = false;
    bool resuming = false;
};

// --- Draw state (ADRs 0038, 0047) -------------------------------------------

struct VertexBufferBinding
{
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
    uint64_t size = 0;
    uint32_t stride = 0;
};

enum class IndexFormat : uint8_t
{
    Uint16,
    Uint32,
};

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

    // Every attachment is cleared, loaded, or discarded; the viewport and
    // scissor start as the full render area.
    void beginRendering(const RenderingDesc& desc);
    void endRendering();

    void bindPipeline(Pipeline* pipeline);
    void bindVertexBuffers(uint32_t firstSlot, std::span<const VertexBufferBinding> bindings);
    void bindIndexBuffer(Buffer* buffer, uint64_t offset, uint64_t size, IndexFormat format);
    void setViewports(std::span<const Viewport> viewports);
    void setScissors(std::span<const Rect> scissors);
    void setBlendConstant(const float constant[4]);
    void setStencilReference(uint8_t reference);
    void setDepthBounds(float minDepth, float maxDepth); // needs GraphicsDeviceCapabilities::depthBounds

    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                     uint32_t firstInstance = 0);
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
