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
class QueryPool;
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
};

// In the RenderTarget layout when the pass begins.
struct ColorAttachment
{
    AttachmentView* view = nullptr;
    LoadOp loadOp = LoadOp::Load;
    StoreOp storeOp = StoreOp::Store;
    ClearValue clearValue;
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

// Indirect argument layouts, tightly packed in the argument buffer; they
// match D3D12's and Vulkan's (ADR 0047). For a draw ID, write each draw's
// index into firstInstance and read SV_StartInstanceLocation in the shader.
struct DrawArguments
{
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstVertex = 0;
    uint32_t firstInstance = 0;
};

struct DrawIndexedArguments
{
    uint32_t indexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstIndex = 0;
    int32_t vertexOffset = 0;
    uint32_t firstInstance = 0;
};

struct DispatchArguments
{
    uint32_t groupCountX = 0;
    uint32_t groupCountY = 0;
    uint32_t groupCountZ = 0;
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
    // begin() starts recording and binds the descriptor heaps; end() makes it
    // submittable; reset() returns it to the initial state once the GPU is
    // done with it, keeping its memory for the next recording.
    Result<void> begin();
    Result<void> end();
    Result<void> reset();

    // Every attachment is cleared, loaded, or discarded; the viewport and
    // scissor start as the full render area.
    void beginRendering(const RenderingDesc& desc);
    void endRendering();

    // Graphics pipelines inside a render pass whose formats they match;
    // compute pipelines outside one (ADRs 0040, 0042).
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

    // Argument buffers are in the IndirectArgument access; offsets are
    // multiples of 4. The ...Count forms read a uint32 draw count from
    // countBuffer and draw at most maxDrawCount.
    void drawIndirect(Buffer* argumentBuffer, uint64_t argumentOffset, uint32_t drawCount);
    void drawIndirectCount(Buffer* argumentBuffer, uint64_t argumentOffset, Buffer* countBuffer, uint64_t countOffset,
                           uint32_t maxDrawCount);
    void drawIndexedIndirect(Buffer* argumentBuffer, uint64_t argumentOffset, uint32_t drawCount);
    void drawIndexedIndirectCount(Buffer* argumentBuffer, uint64_t argumentOffset, Buffer* countBuffer,
                                  uint64_t countOffset, uint32_t maxDrawCount);
    void dispatchIndirect(Buffer* argumentBuffer, uint64_t argumentOffset);
    // Up to limits::maxPushConstantsSize bytes shared by every stage (ADR 0003);
    // offset and size are multiples of 4.
    void pushConstants(const void* data, uint32_t size, uint32_t offset = 0);
    // The root constant buffer (ADR 0044): shaders declare it as
    // `ConstantBuffer<T> name : register(b1);`. address and size are
    // multiples of alignment::constantBuffer; size is at most
    // limits::maxConstantBufferSize. Not bounds-checked on the GPU.
    void setConstantBuffer(uint64_t address, uint32_t size);

    // Outside render passes, with a compute pipeline bound.
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    // Enhanced-Barriers-style sync, access, and layout transitions (ADR 0049),
    // outside render passes. Slag inserts no barriers of its own.
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
    // Occlusion queries (ADR 0051): inside one render pass, in this command
    // buffer, one at a time.
    void beginQuery(QueryPool* pool, uint32_t index);
    void endQuery(QueryPool* pool, uint32_t index);
    // Writes the GPU time once all earlier work on the queue has finished.
    void writeTimestamp(QueryPool* pool, uint32_t index);
    // Writes `count` 64-bit results to the buffer (offset a multiple of 8),
    // which is in the CopyDest access. Outside render passes.
    void resolveQueries(QueryPool* pool, uint32_t first, uint32_t count, Buffer* buffer, uint64_t offset);

    // Debug labels (ADR 0050), shown in capture tools; UTF-8. `color` is an
    // optional RGBA in [0, 1]: Vulkan shows it, D3D12 ignores it. Labels
    // balance within a command buffer, except across a split render pass.
    // Compiled out with SLAG_DISABLE_LABELS.
    void beginLabel(const char* name, const float* color = nullptr);
    void endLabel();
    void insertLabel(const char* name, const float* color = nullptr);

    // Averages an MSAA layer into one mip and layer of a single-sampled
    // texture of the same size and format. Graphics queue only; textures in
    // ResolveSource / ResolveDest; no integer formats (ADR 0048).
    void resolveTexture(Texture* destination, uint32_t destinationMip, uint32_t destinationLayer, Texture* source,
                        uint32_t sourceLayer);

    uint64_t userData() const;
    void setUserData(uint64_t value);

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

protected:
    CommandBuffer() = default;
    ~CommandBuffer() = default;
};

} // namespace slag
