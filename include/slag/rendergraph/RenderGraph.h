#pragma once

// Optional render graph (ADR 0072), built on public Slag functions. Passes
// declare how they use textures and buffers; the graph records every barrier,
// splits the passes into submits with semaphores between queues, begins and
// ends rendering for passes with attachments, and places transient resources
// in shared memory. It never reorders passes: declaration order is execution
// order. Exempt from ADR 0007 (lambdas and templates).

#include <slag/GraphicsDevice.h>
#include <slag/Instance.h>

#include <cstddef>
#include <cstdint>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

namespace slag::rendergraph {

// Handles to the graph's resources, valid for the frame they were made in.
struct TextureHandle
{
    uint32_t index = UINT32_MAX;
};

struct BufferHandle
{
    uint32_t index = UINT32_MAX;
};

// How a pass uses a resource. Shader accesses cover every shader stage of the
// pass's queue type. VertexBuffer, IndexBuffer, ConstantBuffer, and
// IndirectArgument are for buffers only. On copy queues textures stay in the
// Common layout (ADR 0049).
enum class Usage : uint8_t
{
    ShaderRead,
    ShaderWrite, // read-write (RWTexture*, RWBuffer*)
    CopySource,
    CopyDest,
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,
    IndirectArgument,
};

// A texture the graph doesn't own. Every frame states its layout before the
// graph's first submit and the layout the graph leaves it in.
struct ImportedTextureDesc
{
    TextureLayout initialLayout = TextureLayout::Undefined;
    TextureLayout finalLayout = TextureLayout::Common; // not Undefined
    AttachmentView* attachmentView = nullptr;          // needed to use it as an attachment
    Swapchain* swapchain = nullptr; // an acquired swapchain image: waited on by the submit that first uses it
};

class RenderGraph;

// Declares one pass's resource uses, while the pass is added.
class PassBuilder
{
public:
    void use(TextureHandle texture, Usage usage);
    void use(BufferHandle buffer, Usage usage);
    // Graphics passes only; the graph begins and ends rendering around the
    // pass with these attachments, in the order declared.
    void renderTarget(TextureHandle texture, LoadOp loadOp = LoadOp::Load, StoreOp storeOp = StoreOp::Store,
                      const ClearValue& clearValue = {});
    void depthStencil(TextureHandle texture, LoadOp loadOp = LoadOp::Load, StoreOp storeOp = StoreOp::Store,
                      const ClearValue& clearValue = {}, bool readOnly = false);

private:
    friend class RenderGraph;
    PassBuilder(RenderGraph* graph, uint32_t pass)
        : graph(graph)
        , pass(pass)
    {}

    RenderGraph* graph = nullptr;
    uint32_t pass = 0;
};

// Given to a pass's record function.
class PassContext
{
public:
    Texture* texture(TextureHandle texture) const;
    Buffer* buffer(BufferHandle buffer) const;
    // 0 to framesInFlight - 1: which copy of per-frame data (descriptor
    // slots, constants) the pass may write.
    uint32_t frameSlot() const;

private:
    friend class RenderGraph;
    explicit PassContext(const RenderGraph* graph)
        : graph(graph)
    {}

    const RenderGraph* graph = nullptr;
};

struct RenderGraphDesc
{
    GraphicsDevice* device = nullptr;
    // Null: the device's first queue of the type. Swapchains the graph
    // presents images of are created on the graphics queue.
    CommandQueue* graphicsQueue = nullptr;
    CommandQueue* computeQueue = nullptr;
    CommandQueue* copyQueue = nullptr;
    uint32_t framesInFlight = 2;
    AllocatorCallbacks allocatorCallbacks;    // the graph's own CPU memory
    DebugMessageFn messageCallback = nullptr; // usage errors; null: stderr
    void* messageUserData = nullptr;
};

// One frame at a time: beginFrame, declare resources and passes, compile,
// recordPass for every pass (from any threads, each pass once), execute.
// The calls other than recordPass are not thread-safe.
class RenderGraph
{
public:
    // Waits until the frame that last used this frame slot has finished,
    // then starts declaring a new frame.
    Result<void> beginFrame();

    TextureHandle importTexture(Texture* texture, const ImportedTextureDesc& desc = {});
    BufferHandle importBuffer(Buffer* buffer);
    // Transient: lives for this frame, in memory shared with transients whose
    // uses don't overlap. Contents start undefined: the first use writes (or
    // clears) it. The placement field is ignored; castable formats aren't
    // supported.
    TextureHandle createTransientTexture(const Texture2DDesc& desc);
    BufferHandle createTransientBuffer(const BufferDesc& desc);

    // `setup(PassBuilder&)` runs now; `record(PassContext&, CommandBuffer*)` is
    // stored (in the graph's memory) and runs in recordPass.
    template <class Setup, class Record>
    void addPass(const char* name, CommandQueueType queue, Setup&& setup, Record&& record)
    {
        using Stored = std::decay_t<Record>;
        void* memory = allocatePassData(sizeof(Stored), alignof(Stored));
        Stored* stored = memory ? new (memory) Stored(std::forward<Record>(record)) : nullptr;
        PassBuilder builder = beginPass(
            name, queue, stored,
            [](void* data, PassContext& context, CommandBuffer* commandBuffer) {
                (*static_cast<Stored*>(data))(context, commandBuffer);
            },
            [](void* data) { static_cast<Stored*>(data)->~Stored(); });
        setup(builder);
    }

    // Works out barriers, submits, and transient placement; creates the
    // transient resources and command buffers it needs. Usage errors are
    // reported and return Unsupported.
    Result<void> compile();

    uint32_t passCount() const;
    // Records pass `index` into its own command buffer. Different passes may
    // be recorded concurrently.
    Result<void> recordPass(uint32_t index);
    // Submits every recorded pass, in order, with the semaphores between queues.
    Result<void> execute();

    // Bytes of transient memory this frame's resources share.
    uint64_t transientMemorySize() const;

    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

protected:
    RenderGraph() = default;
    ~RenderGraph() = default;

private:
    friend class PassBuilder;
    friend class PassContext;
    using RecordFn = void (*)(void* data, PassContext& context, CommandBuffer* commandBuffer);
    using DestroyFn = void (*)(void* data);
    void* allocatePassData(size_t size, size_t alignment);
    PassBuilder beginPass(const char* name, CommandQueueType queue, void* data, RecordFn record, DestroyFn destroy);
};

Result<RenderGraph*> createRenderGraph(const RenderGraphDesc& desc);
// Waits for the graph's work, then frees it and everything it created.
void destroy(RenderGraph* graph);

} // namespace slag::rendergraph
