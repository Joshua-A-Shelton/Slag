#include "common/HostAllocator.h"
#include "common/Vector.h"
#include "rendergraph/Plan.h"

#include <slag/rendergraph/RenderGraph.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <utility>

namespace slag::rendergraph {

namespace {

enum class UseKind : uint8_t
{
    Usage,
    RenderTarget,
    DepthWrite,
    DepthRead,
};

struct DeclaredUse
{
    uint32_t pass = 0;
    uint32_t resource = 0;
    UseKind kind = UseKind::Usage;
    Usage usage = Usage::ShaderRead;
    SubresourceRange range; // textures
};

struct Attachment
{
    uint32_t resource = none;
    LoadOp loadOp = LoadOp::Load;
    StoreOp storeOp = StoreOp::Store;
    ClearValue clearValue;
    uint32_t mip = 0;
    uint32_t baseLayer = 0;
    uint32_t layerCount = 1;
    AttachmentView* view = nullptr; // resolved by compile
};

enum class TransientKind : uint8_t
{
    Texture2D,
    Texture2DArray,
    Cube,
    CubeArray,
    Texture3D,
};

// A transient texture's description, whichever create function made it.
struct TransientTexture
{
    TransientKind kind = TransientKind::Texture2D;
    PixelFormat format = PixelFormat::Undefined;
    uint32_t width = 1; // cubes: the face size
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t count = 1; // array layers, or cubes
    uint32_t mipCount = 1;
    uint32_t sampleCount = 1;
    TextureUsage usage = TextureUsage::None;
    const char* debugName = nullptr;

    uint32_t layerCount() const
    {
        switch (kind)
        {
        case TransientKind::Texture2DArray:
            return count;
        case TransientKind::Cube:
            return 6;
        case TransientKind::CubeArray:
            return 6 * count;
        default:
            return 1;
        }
    }

    bool operator==(const TransientTexture& other) const
    {
        return kind == other.kind && format == other.format && width == other.width && height == other.height &&
               depth == other.depth && count == other.count && mipCount == other.mipCount &&
               sampleCount == other.sampleCount && usage == other.usage;
    }
};

PlacementInfo placementOf(GraphicsDevice* device, const TransientTexture& t)
{
    switch (t.kind)
    {
    case TransientKind::Texture2D:
        return device->placementInfo(Texture2DDesc{.format = t.format,
                                                   .width = t.width,
                                                   .height = t.height,
                                                   .mipCount = t.mipCount,
                                                   .sampleCount = t.sampleCount,
                                                   .usage = t.usage});
    case TransientKind::Texture2DArray:
        return device->placementInfo(Texture2DArrayDesc{.format = t.format,
                                                        .width = t.width,
                                                        .height = t.height,
                                                        .layerCount = t.count,
                                                        .mipCount = t.mipCount,
                                                        .sampleCount = t.sampleCount,
                                                        .usage = t.usage});
    case TransientKind::Cube:
        return device->placementInfo(
            TextureCubeDesc{.format = t.format, .size = t.width, .mipCount = t.mipCount, .usage = t.usage});
    case TransientKind::CubeArray:
        return device->placementInfo(TextureCubeArrayDesc{
            .format = t.format, .size = t.width, .cubeCount = t.count, .mipCount = t.mipCount, .usage = t.usage});
    case TransientKind::Texture3D:
        return device->placementInfo(Texture3DDesc{.format = t.format,
                                                   .width = t.width,
                                                   .height = t.height,
                                                   .depth = t.depth,
                                                   .mipCount = t.mipCount,
                                                   .usage = t.usage});
    }
    return {};
}

Result<Texture*> createTransient(GraphicsDevice* device, const TransientTexture& t, const ClearValue* clearValue,
                                 const Placement& placement)
{
    switch (t.kind)
    {
    case TransientKind::Texture2D:
        return device->createTexture2D({.format = t.format,
                                        .width = t.width,
                                        .height = t.height,
                                        .mipCount = t.mipCount,
                                        .sampleCount = t.sampleCount,
                                        .usage = t.usage,
                                        .clearValue = clearValue,
                                        .placement = placement,
                                        .debugName = t.debugName});
    case TransientKind::Texture2DArray:
        return device->createTexture2DArray({.format = t.format,
                                             .width = t.width,
                                             .height = t.height,
                                             .layerCount = t.count,
                                             .mipCount = t.mipCount,
                                             .sampleCount = t.sampleCount,
                                             .usage = t.usage,
                                             .clearValue = clearValue,
                                             .placement = placement,
                                             .debugName = t.debugName});
    case TransientKind::Cube:
        return device->createTextureCube({.format = t.format,
                                          .size = t.width,
                                          .mipCount = t.mipCount,
                                          .usage = t.usage,
                                          .clearValue = clearValue,
                                          .placement = placement,
                                          .debugName = t.debugName});
    case TransientKind::CubeArray:
        return device->createTextureCubeArray({.format = t.format,
                                               .size = t.width,
                                               .cubeCount = t.count,
                                               .mipCount = t.mipCount,
                                               .usage = t.usage,
                                               .clearValue = clearValue,
                                               .placement = placement,
                                               .debugName = t.debugName});
    case TransientKind::Texture3D:
        return device->createTexture3D({.format = t.format,
                                        .width = t.width,
                                        .height = t.height,
                                        .depth = t.depth,
                                        .mipCount = t.mipCount,
                                        .usage = t.usage,
                                        .clearValue = clearValue,
                                        .placement = placement,
                                        .debugName = t.debugName});
    }
    return Status::Unsupported;
}

struct ResourceEntry
{
    bool texture = true;
    bool transient = false;
    Texture* textureObject = nullptr; // imported, or the transient's object once compiled
    Buffer* bufferObject = nullptr;
    AttachmentView* view = nullptr;
    ImportedTextureDesc import;
    TransientTexture shape; // transient textures
    uint32_t mipCount = 1;  // textures, set by compile
    uint32_t layerCount = 1;
    uint32_t depth = 1;
    ClearValue clearValue;
    bool hasClearValue = false;
    BufferDesc bufferDesc;
};

struct PassEntry
{
    const char* name = nullptr;
    CommandQueueType queue = CommandQueueType::Graphics;
    void* data = nullptr;
    void (*record)(void*, PassContext&, CommandBuffer*) = nullptr;
    void (*destroy)(void*) = nullptr;
    Attachment colors[limits::maxColorTargets];
    uint32_t colorCount = 0;
    Attachment depth;
    bool depthReadOnly = false;
    CommandBuffer* commandBuffer = nullptr;
    // Converted from the plan by compile; read by recordPass.
    Vector<TextureBarrier> beginTextures;
    Vector<BufferBarrier> beginBuffers;
    Vector<TextureBarrier> endTextures;
    Vector<BufferBarrier> endBuffers;
    bool recorded = false;
};

struct CachedTexture
{
    TransientTexture shape;
    ClearValue clearValue;
    bool hasClearValue = false;
    uint64_t offset = 0;
    Texture* texture = nullptr;
    bool used = false;
};

// An attachment view of a cached transient texture: one mip, some layers.
struct CachedView
{
    Texture* texture = nullptr;
    uint32_t mip = 0;
    uint32_t baseLayer = 0;
    uint32_t layerCount = 1;
    AttachmentView* view = nullptr;
};

struct CachedBuffer
{
    uint64_t size = 0;
    uint64_t offset = 0;
    Buffer* buffer = nullptr;
    bool used = false;
};

// What one frame in flight owns; reused once that frame has finished.
struct FrameSlot
{
    MemoryBlock* block = nullptr;
    uint64_t blockSize = 0;
    Vector<CachedTexture> textures;
    Vector<CachedView> views;
    Vector<CachedBuffer> buffers;
    Vector<CommandBuffer*> commandBuffers[3];
    uint32_t usedCommandBuffers[3] = {};
    uint64_t waitValues[3] = {};
};

struct ArenaChunk
{
    void* memory = nullptr;
    size_t size = 0;
    size_t used = 0;
};

size_t queueIndex(CommandQueueType queue)
{
    return static_cast<size_t>(queue);
}

const char* queueName(CommandQueueType queue)
{
    switch (queue)
    {
    case CommandQueueType::Graphics:
        return "graphics";
    case CommandQueueType::Compute:
        return "compute";
    case CommandQueueType::Copy:
        return "copy";
    }
    return "?";
}

bool sameTransient(const CachedTexture& cached, const ResourceEntry& entry)
{
    bool clearMatches = cached.hasClearValue == entry.hasClearValue;
    for (int i = 0; clearMatches && entry.hasClearValue && i < 4; ++i)
    {
        clearMatches = cached.clearValue.color[i] == entry.clearValue.color[i];
    }
    clearMatches = clearMatches && (!entry.hasClearValue || (cached.clearValue.depth == entry.clearValue.depth &&
                                                             cached.clearValue.stencil == entry.clearValue.stencil));
    return clearMatches && cached.shape == entry.shape;
}

} // namespace

struct RenderGraphImpl : RenderGraph
{
    explicit RenderGraphImpl(const RenderGraphDesc& desc)
        : allocator(desc.allocatorCallbacks)
        , device(desc.device)
        , callback(desc.messageCallback)
        , callbackUserData(desc.messageUserData)
        , framesInFlight(desc.framesInFlight ? desc.framesInFlight : 1)
        , slots(&allocator)
        , resources(&allocator)
        , passes(&allocator)
        , declaredUses(&allocator)
        , chunks(&allocator)
        , planResources(&allocator)
        , planPasses(&allocator)
        , planUses(&allocator)
        , plan(&allocator)
        , batchValues(&allocator)
        , batchSwapchains(&allocator)
        , submitCommandBuffers(&allocator)
        , submitWaits(&allocator)
        , submitSwapchains(&allocator)
        , defragmentMoves(&allocator)
        , defragmentLayouts(&allocator)
    {
        queues[0] = desc.graphicsQueue ? desc.graphicsQueue : device->commandQueue(CommandQueueType::Graphics);
        queues[1] = desc.computeQueue ? desc.computeQueue : device->commandQueue(CommandQueueType::Compute);
        queues[2] = desc.copyQueue ? desc.copyQueue : device->commandQueue(CommandQueueType::Copy);
    }

    void report(const char* format, ...)
    {
        char text[512];
        va_list args;
        va_start(args, format);
        std::vsnprintf(text, sizeof(text), format, args);
        va_end(args);
        if (callback)
        {
            DebugMessage message;
            message.text = text;
            callback(message, callbackUserData);
        }
        else
        {
            std::fprintf(stderr, "%s\n", text);
        }
        errors = true;
    }

    bool declaring(const char* function)
    {
        if (!frameOpen)
        {
            report("slag::rendergraph: %s: call beginFrame first, and declare before compile", function);
            return false;
        }
        return true;
    }

    uint32_t addResource(ResourceEntry&& entry)
    {
        if (!resources.pushBack(std::move(entry)))
        {
            outOfMemory = true;
            return none;
        }
        return static_cast<uint32_t>(resources.size() - 1);
    }

    void addUse(uint32_t pass, uint32_t resource, bool texture, UseKind kind, Usage usage,
                const SubresourceRange& range, const char* function)
    {
        if (resource >= resources.size() || resources[resource].texture != texture)
        {
            report("slag::rendergraph: %s: pass \"%s\": not a %s of this frame", function, passes[pass].name,
                   texture ? "texture" : "buffer");
            return;
        }
        DeclaredUse use;
        use.pass = pass;
        use.resource = resource;
        use.kind = kind;
        use.usage = usage;
        use.range = range;
        outOfMemory = outOfMemory || !declaredUses.pushBack(use);
    }

    // Sync, access, layout, and write for one declared use on a queue type.
    bool describe(const DeclaredUse& use, CommandQueueType queue, PlanUse& out)
    {
        const char* name = passes[use.pass].name;
        BarrierSync shaderSync =
            queue == CommandQueueType::Graphics ? BarrierSync::AllShading : BarrierSync::ComputeShading;
        out.resource = use.resource;
        out.write = false;
        out.layout = TextureLayout::Common;
        bool texture = resources[use.resource].texture;
        switch (use.kind)
        {
        case UseKind::RenderTarget:
            out = {use.resource, BarrierSync::RenderTarget, BarrierAccess::RenderTarget, TextureLayout::RenderTarget,
                   true};
            return true;
        case UseKind::DepthWrite:
            out = {use.resource, BarrierSync::DepthStencil, BarrierAccess::DepthStencilWrite,
                   TextureLayout::DepthStencilWrite, true};
            return true;
        case UseKind::DepthRead:
            out = {use.resource, BarrierSync::DepthStencil, BarrierAccess::DepthStencilRead,
                   TextureLayout::DepthStencilRead, false};
            return true;
        case UseKind::Usage:
            break;
        }
        if (queue == CommandQueueType::Copy && use.usage != Usage::CopySource && use.usage != Usage::CopyDest)
        {
            report("slag::rendergraph: pass \"%s\": copy queues only copy", name);
            return false;
        }
        bool bufferOnly = use.usage == Usage::VertexBuffer || use.usage == Usage::IndexBuffer ||
                          use.usage == Usage::ConstantBuffer || use.usage == Usage::IndirectArgument;
        if (texture && bufferOnly)
        {
            report("slag::rendergraph: pass \"%s\": that usage is for buffers only", name);
            return false;
        }
        if ((use.usage == Usage::VertexBuffer || use.usage == Usage::IndexBuffer) &&
            queue != CommandQueueType::Graphics)
        {
            report("slag::rendergraph: pass \"%s\": vertex and index buffers are for graphics passes", name);
            return false;
        }
        bool copyQueue = queue == CommandQueueType::Copy;
        switch (use.usage)
        {
        case Usage::ShaderRead:
            out.sync = shaderSync;
            out.access = BarrierAccess::ShaderRead;
            out.layout = TextureLayout::ShaderRead;
            break;
        case Usage::ShaderWrite:
            out.sync = shaderSync;
            out.access = BarrierAccess::ShaderWrite;
            out.layout = TextureLayout::ShaderWrite;
            out.write = true;
            break;
        case Usage::CopySource:
            out.sync = BarrierSync::Copy;
            out.access = BarrierAccess::CopySource;
            out.layout = copyQueue ? TextureLayout::Common : TextureLayout::CopySource;
            break;
        case Usage::CopyDest:
            out.sync = BarrierSync::Copy;
            out.access = BarrierAccess::CopyDest;
            out.layout = copyQueue ? TextureLayout::Common : TextureLayout::CopyDest;
            out.write = true;
            break;
        case Usage::VertexBuffer:
            out.sync = BarrierSync::IndexInput | BarrierSync::VertexShading;
            out.access = BarrierAccess::VertexBuffer;
            break;
        case Usage::IndexBuffer:
            out.sync = BarrierSync::IndexInput;
            out.access = BarrierAccess::IndexBuffer;
            break;
        case Usage::ConstantBuffer:
            out.sync = shaderSync;
            out.access = BarrierAccess::ConstantBuffer;
            break;
        case Usage::IndirectArgument:
            out.sync = BarrierSync::ExecuteIndirect;
            out.access = BarrierAccess::IndirectArgument;
            break;
        }
        return true;
    }

    // Merges this pass's declared uses, one per resource, into planUses.
    bool mergeUses(uint32_t p)
    {
        PassEntry& pass = passes[p];
        PlanPass planPass;
        planPass.queue = pass.queue;
        planPass.firstUse = static_cast<uint32_t>(planUses.size());
        for (const DeclaredUse& declared : declaredUses)
        {
            if (declared.pass != p)
            {
                continue;
            }
            PlanUse use;
            if (!describe(declared, pass.queue, use))
            {
                continue;
            }
            const ResourceEntry& entry = resources[use.resource];
            if (entry.texture)
            {
                SubresourceRange range = declared.range;
                if (declared.kind != UseKind::Usage && entry.depth > 1)
                {
                    range.baseLayer = 0; // 3D: attachment layers are depth slices, tracked with their mip
                    range.layerCount = 1;
                }
                uint32_t mips = range.mipCount == SubresourceRange::remainingMips && range.baseMip < entry.mipCount
                                    ? entry.mipCount - range.baseMip
                                    : range.mipCount;
                uint32_t layers =
                    range.layerCount == SubresourceRange::remainingLayers && range.baseLayer < entry.layerCount
                        ? entry.layerCount - range.baseLayer
                        : range.layerCount;
                if (range.baseMip >= entry.mipCount || mips == 0 || mips > entry.mipCount - range.baseMip ||
                    range.baseLayer >= entry.layerCount || layers == 0 || layers > entry.layerCount - range.baseLayer)
                {
                    report("slag::rendergraph: pass \"%s\": a range is outside its texture (%u mips, %u layers)",
                           pass.name, entry.mipCount, entry.layerCount);
                    continue;
                }
                use.baseMip = range.baseMip;
                use.mipCount = mips;
                use.baseLayer = range.baseLayer;
                use.layerCount = layers;
            }
            bool merged = false;
            for (uint32_t i = planPass.firstUse; i < planUses.size(); ++i)
            {
                PlanUse& existing = planUses[i];
                if (existing.resource != use.resource)
                {
                    continue;
                }
                if (entry.texture && (existing.baseMip != use.baseMip || existing.mipCount != use.mipCount ||
                                      existing.baseLayer != use.baseLayer || existing.layerCount != use.layerCount))
                {
                    bool overlap = use.baseMip < existing.baseMip + existing.mipCount &&
                                   existing.baseMip < use.baseMip + use.mipCount &&
                                   use.baseLayer < existing.baseLayer + existing.layerCount &&
                                   existing.baseLayer < use.baseLayer + use.layerCount;
                    if (overlap)
                    {
                        report("slag::rendergraph: pass \"%s\": two uses of a texture overlap without covering the "
                               "same subresources",
                               pass.name);
                        return false;
                    }
                    continue; // disjoint: a use of its own
                }
                merged = true;
                if (resources[use.resource].texture && existing.layout != use.layout)
                {
                    // Sampling a read-only depth attachment shares its layout.
                    bool depthRead =
                        (existing.layout == TextureLayout::DepthStencilRead &&
                         use.layout == TextureLayout::ShaderRead) ||
                        (use.layout == TextureLayout::DepthStencilRead && existing.layout == TextureLayout::ShaderRead);
                    if (!depthRead)
                    {
                        report("slag::rendergraph: pass \"%s\": a texture is used in two layouts in one pass",
                               pass.name);
                        return false;
                    }
                    existing.layout = TextureLayout::DepthStencilRead;
                }
                existing.sync = existing.sync | use.sync;
                existing.access = existing.access | use.access;
                existing.write = existing.write || use.write;
            }
            if (!merged && !planUses.pushBack(use))
            {
                outOfMemory = true;
                return false;
            }
        }
        planPass.useCount = static_cast<uint32_t>(planUses.size()) - planPass.firstUse;
        outOfMemory = outOfMemory || !planPasses.pushBack(planPass);
        return true;
    }

    CommandBuffer* takeCommandBuffer(CommandQueueType queue)
    {
        FrameSlot& frame = slots[slot];
        size_t q = queueIndex(queue);
        if (frame.usedCommandBuffers[q] < frame.commandBuffers[q].size())
        {
            return frame.commandBuffers[q][frame.usedCommandBuffers[q]++];
        }
        Result<CommandBuffer*> created = device->createCommandBuffer({.queueType = queue, .debugName = "render graph"});
        if (!created || !frame.commandBuffers[q].pushBack(created.value()))
        {
            if (created)
            {
                device->destroy(created.value());
            }
            return nullptr;
        }
        ++frame.usedCommandBuffers[q];
        return created.value();
    }

    void destroyCached(FrameSlot& frame, bool unusedOnly)
    {
        size_t kept = 0;
        for (size_t i = 0; i < frame.textures.size(); ++i)
        {
            CachedTexture& cached = frame.textures[i];
            if (unusedOnly && cached.used)
            {
                frame.textures[kept++] = cached;
                continue;
            }
            // Its views go first (Slag requires it).
            size_t keptViews = 0;
            for (size_t v = 0; v < frame.views.size(); ++v)
            {
                if (frame.views[v].texture == cached.texture)
                {
                    device->destroy(frame.views[v].view);
                    continue;
                }
                frame.views[keptViews++] = frame.views[v];
            }
            while (frame.views.size() > keptViews)
            {
                frame.views.popBack();
            }
            device->destroy(cached.texture);
        }
        while (frame.textures.size() > kept)
        {
            frame.textures.popBack();
        }
        kept = 0;
        for (size_t i = 0; i < frame.buffers.size(); ++i)
        {
            CachedBuffer& cached = frame.buffers[i];
            if (unusedOnly && cached.used)
            {
                frame.buffers[kept++] = cached;
                continue;
            }
            device->destroy(cached.buffer);
        }
        while (frame.buffers.size() > kept)
        {
            frame.buffers.popBack();
        }
    }

    Status placeTransients()
    {
        FrameSlot& frame = slots[slot];
        if (plan.transientSize > frame.blockSize)
        {
            // Everything placed in the old block goes with it.
            destroyCached(frame, false);
            device->destroy(frame.block);
            frame.block = nullptr;
            frame.blockSize = 0;
            // Whole 64 KiB pages, the D3D12 heap granularity.
            uint64_t size = (plan.transientSize + 65535) / 65536 * 65536;
            Result<MemoryBlock*> block =
                device->createMemoryBlock({.size = size, .debugName = "render graph transients"});
            if (!block)
            {
                return block.status();
            }
            frame.block = block.value();
            frame.blockSize = size;
        }
        for (CachedTexture& cached : frame.textures)
        {
            cached.used = false;
        }
        for (CachedBuffer& cached : frame.buffers)
        {
            cached.used = false;
        }
        for (uint32_t r = 0; r < resources.size(); ++r)
        {
            ResourceEntry& entry = resources[r];
            if (!entry.transient || plan.firstPass[r] == none)
            {
                continue;
            }
            uint64_t offset = plan.offsets[r];
            if (entry.texture)
            {
                CachedTexture* match = nullptr;
                for (CachedTexture& cached : frame.textures)
                {
                    if (!cached.used && cached.offset == offset && sameTransient(cached, entry))
                    {
                        match = &cached;
                        break;
                    }
                }
                if (!match)
                {
                    CachedTexture cached;
                    cached.shape = entry.shape;
                    cached.clearValue = entry.clearValue;
                    cached.hasClearValue = entry.hasClearValue;
                    cached.offset = offset;
                    Result<Texture*> texture =
                        createTransient(device, entry.shape, entry.hasClearValue ? &entry.clearValue : nullptr,
                                        Placement::placed(frame.block, offset));
                    if (!texture)
                    {
                        return texture.status();
                    }
                    cached.texture = texture.value();
                    if (!frame.textures.pushBack(cached))
                    {
                        device->destroy(cached.texture);
                        return Status::OutOfHostMemory;
                    }
                    match = &frame.textures[frame.textures.size() - 1];
                }
                match->used = true;
                entry.textureObject = match->texture;
            }
            else
            {
                CachedBuffer* match = nullptr;
                for (CachedBuffer& cached : frame.buffers)
                {
                    if (!cached.used && cached.offset == offset && cached.size == entry.bufferDesc.size)
                    {
                        match = &cached;
                        break;
                    }
                }
                if (!match)
                {
                    BufferDesc desc = entry.bufferDesc;
                    desc.placement = Placement::placed(frame.block, offset);
                    Result<Buffer*> buffer = device->createBuffer(desc);
                    if (!buffer)
                    {
                        return buffer.status();
                    }
                    CachedBuffer cached;
                    cached.size = desc.size;
                    cached.offset = offset;
                    cached.buffer = buffer.value();
                    if (!frame.buffers.pushBack(cached))
                    {
                        device->destroy(cached.buffer);
                        return Status::OutOfHostMemory;
                    }
                    match = &frame.buffers[frame.buffers.size() - 1];
                }
                match->used = true;
                entry.bufferObject = match->buffer;
            }
        }
        // The slot's previous frame has finished (beginFrame), so unused
        // objects can go now.
        destroyCached(frame, true);
        return Status::Ok;
    }

    // The attachment view of one mip and some layers of a texture: the
    // user's for an imported one, a cached one for a transient.
    Result<AttachmentView*> attachmentView(const Attachment& attachment)
    {
        const ResourceEntry& entry = resources[attachment.resource];
        if (!entry.transient)
        {
            return entry.view;
        }
        FrameSlot& frame = slots[slot];
        for (const CachedView& cached : frame.views)
        {
            if (cached.texture == entry.textureObject && cached.mip == attachment.mip &&
                cached.baseLayer == attachment.baseLayer && cached.layerCount == attachment.layerCount)
            {
                return cached.view;
            }
        }
        Result<AttachmentView*> view = device->createAttachmentView({.texture = entry.textureObject,
                                                                     .mip = attachment.mip,
                                                                     .baseLayer = attachment.baseLayer,
                                                                     .layerCount = attachment.layerCount});
        if (!view)
        {
            return view;
        }
        if (!frame.views.pushBack(
                {entry.textureObject, attachment.mip, attachment.baseLayer, attachment.layerCount, view.value()}))
        {
            device->destroy(view.value());
            return Status::OutOfHostMemory;
        }
        return view;
    }

    bool convert(const Vector<PlanBarrier>& barriers, Vector<TextureBarrier>& textures, Vector<BufferBarrier>& buffers)
    {
        textures.clear();
        buffers.clear();
        for (const PlanBarrier& barrier : barriers)
        {
            const ResourceEntry& entry = resources[barrier.resource];
            if (entry.texture)
            {
                TextureBarrier converted = {.syncBefore = barrier.syncBefore,
                                            .syncAfter = barrier.syncAfter,
                                            .accessBefore = barrier.accessBefore,
                                            .accessAfter = barrier.accessAfter,
                                            .layoutBefore = barrier.layoutBefore,
                                            .layoutAfter = barrier.layoutAfter,
                                            .texture = entry.textureObject,
                                            .range = {.baseMip = barrier.baseMip,
                                                      .mipCount = barrier.mipCount,
                                                      .baseLayer = barrier.baseLayer,
                                                      .layerCount = barrier.layerCount}};
                if (!textures.pushBack(converted))
                {
                    return false;
                }
            }
            else
            {
                BufferBarrier converted = {.syncBefore = barrier.syncBefore,
                                           .syncAfter = barrier.syncAfter,
                                           .accessBefore = barrier.accessBefore,
                                           .accessAfter = barrier.accessAfter,
                                           .buffer = entry.bufferObject};
                if (!buffers.pushBack(converted))
                {
                    return false;
                }
            }
        }
        return true;
    }

    void releaseDefragmentRequest()
    {
        if (layoutData && layoutDestroy)
        {
            layoutDestroy(layoutData);
        }
        layoutData = nullptr;
        layoutOf = nullptr;
        layoutDestroy = nullptr;
        defragmentRequested = false;
    }

    // Records the pending pass's copies in their own submit on the graphics
    // queue, after all of the graph's work, waits for them, and ends the pass.
    // Either step may fail (out of memory); the pass then stays pending and
    // beginFrame continues it.
    Result<void> continueDefragmentation()
    {
        if (!pendingRecorded)
        {
            std::span<const DefragmentationMove> moves = pendingPass->moves();
            defragmentMoves.clear();
            defragmentLayouts.clear();
            if (!defragmentMoves.reserve(moves.size()) || !defragmentLayouts.reserve(moves.size()))
            {
                return Status::OutOfHostMemory;
            }
            for (const DefragmentationMove& move : moves)
            {
                (void)defragmentMoves.pushBack(move);
                TextureLayout layout = TextureLayout::Common; // ignored for buffers
                if (move.texture)
                {
                    bool imported = false;
                    for (const ResourceEntry& entry : resources)
                    {
                        if (!imported && entry.texture && !entry.transient && entry.textureObject == move.texture)
                        {
                            layout = entry.import.finalLayout; // where this frame left it
                            imported = true;
                        }
                    }
                    layout = imported ? layout : layoutOf(layoutData, move.texture);
                }
                (void)defragmentLayouts.pushBack(layout);
            }
            pendingCompleted = {};
            if (!moves.empty())
            {
                CommandBuffer* commandBuffer = takeCommandBuffer(CommandQueueType::Graphics);
                if (!commandBuffer)
                {
                    return Status::OutOfHostMemory;
                }
                Result<void> begun = commandBuffer->begin();
                if (!begun)
                {
                    return begun;
                }
                commandBuffer->beginLabel("defragmentation");
                pendingPass->recordCopies(commandBuffer, {defragmentLayouts.data(), defragmentLayouts.size()});
                commandBuffer->endLabel();
                Result<void> ended = commandBuffer->end();
                if (!ended)
                {
                    return ended;
                }
                // After every queue's graph work: nothing earlier in this
                // submit uses a moved resource (ADR 0062).
                submitWaits.clear();
                for (size_t q = 1; q < 3; ++q)
                {
                    if (semaphoreValues[q] != 0 && !submitWaits.pushBack({semaphores[q], semaphoreValues[q]}))
                    {
                        return Status::OutOfHostMemory;
                    }
                }
                uint64_t value = semaphoreValues[0] + 1;
                SemaphoreValue signal = {semaphores[0], value};
                CommandBuffer* list[] = {commandBuffer};
                Result<void> submitted = queues[0]->submit({.waits = {submitWaits.data(), submitWaits.size()},
                                                            .commandBuffers = list,
                                                            .signals = {&signal, 1}});
                if (!submitted)
                {
                    return submitted;
                }
                semaphoreValues[0] = value;
                slots[slot].waitValues[0] = value;
                pendingCompleted = signal;
                Result<void> waited = semaphores[0]->wait(value, UINT64_MAX);
                if (!waited)
                {
                    return waited;
                }
            }
            pendingRecorded = true;
        }
        Result<void> ended = device->endDefragmentation(pendingPass, pendingCompleted);
        if (!ended)
        {
            return ended;
        }
        pendingPass = nullptr;
        return {};
    }

    void destroyPassData()
    {
        for (PassEntry& pass : passes)
        {
            if (pass.data && pass.destroy)
            {
                pass.destroy(pass.data);
            }
        }
        passes.clear();
        for (ArenaChunk& chunk : chunks)
        {
            chunk.used = 0;
        }
    }

    HostAllocator allocator;
    GraphicsDevice* device = nullptr;
    DebugMessageFn callback = nullptr;
    void* callbackUserData = nullptr;
    uint32_t framesInFlight = 2;
    CommandQueue* queues[3] = {};
    Semaphore* semaphores[3] = {};
    uint64_t semaphoreValues[3] = {};
    uint64_t frameNumber = 0;
    uint32_t slot = 0;
    bool frameOpen = false;
    bool compiled = false;
    bool errors = false;
    bool outOfMemory = false;

    Vector<FrameSlot> slots;
    Vector<ResourceEntry> resources;
    Vector<PassEntry> passes;
    Vector<DeclaredUse> declaredUses;
    Vector<ArenaChunk> chunks;
    Vector<PlanResource> planResources;
    Vector<PlanPass> planPasses;
    Vector<PlanUse> planUses;
    Plan plan;
    Vector<uint64_t> batchValues;     // per batch: the value it signals on its queue's semaphore
    Vector<uint32_t> batchSwapchains; // per batch: index of the first acquired swapchain entry, or none
    CommandBuffer* epilogueCommandBuffer = nullptr;
    Vector<TextureBarrier> epilogueBarriers;
    Vector<CommandBuffer*> submitCommandBuffers;
    Vector<SemaphoreValue> submitWaits;
    Vector<Swapchain*> submitSwapchains;

    // Defragmentation (ADR 0062): the request for this frame, and a pass whose
    // copies or end ran out of memory.
    bool defragmentRequested = false;
    DefragmentationDesc defragmentDesc;
    void* layoutData = nullptr;
    TextureLayout (*layoutOf)(void*, Texture*) = nullptr;
    void (*layoutDestroy)(void*) = nullptr;
    DefragmentationPass* pendingPass = nullptr;
    bool pendingRecorded = false;
    SemaphoreValue pendingCompleted;
    Vector<DefragmentationMove> defragmentMoves;
    Vector<TextureLayout> defragmentLayouts;
};

namespace {

RenderGraphImpl* impl(RenderGraph* graph)
{
    return static_cast<RenderGraphImpl*>(graph);
}

const RenderGraphImpl* impl(const RenderGraph* graph)
{
    return static_cast<const RenderGraphImpl*>(graph);
}

} // namespace

// --- PassBuilder / PassContext ------------------------------------------------

void PassBuilder::use(RenderGraphTexture texture, Usage usage, const SubresourceRange& range)
{
    if (!graph)
    {
        return; // addPass failed and reported
    }
    impl(graph)->addUse(pass, texture.index, true, UseKind::Usage, usage, range, "PassBuilder::use");
}

void PassBuilder::use(RenderGraphBuffer buffer, Usage usage)
{
    if (!graph)
    {
        return;
    }
    impl(graph)->addUse(pass, buffer.index, false, UseKind::Usage, usage, {}, "PassBuilder::use");
}

void PassBuilder::renderTarget(RenderGraphTexture texture, LoadOp loadOp, StoreOp storeOp, const ClearValue& clearValue,
                               uint32_t mip, uint32_t baseLayer, uint32_t layerCount)
{
    if (!graph)
    {
        return;
    }
    RenderGraphImpl* self = impl(graph);
    PassEntry& entry = self->passes[pass];
    if (entry.queue != CommandQueueType::Graphics || entry.colorCount == limits::maxColorTargets)
    {
        self->report("slag::rendergraph: PassBuilder::renderTarget: pass \"%s\": %s", entry.name,
                     entry.queue != CommandQueueType::Graphics ? "attachments are for graphics passes"
                                                               : "too many render targets");
        return;
    }
    entry.colors[entry.colorCount++] = {texture.index, loadOp, storeOp, clearValue, mip, baseLayer, layerCount};
    self->addUse(pass, texture.index, true, UseKind::RenderTarget, Usage::ShaderRead,
                 {.baseMip = mip, .mipCount = 1, .baseLayer = baseLayer, .layerCount = layerCount},
                 "PassBuilder::renderTarget");
}

void PassBuilder::depthStencil(RenderGraphTexture texture, LoadOp loadOp, StoreOp storeOp, const ClearValue& clearValue,
                               bool readOnly, uint32_t mip, uint32_t baseLayer, uint32_t layerCount)
{
    if (!graph)
    {
        return;
    }
    RenderGraphImpl* self = impl(graph);
    PassEntry& entry = self->passes[pass];
    if (entry.queue != CommandQueueType::Graphics || entry.depth.resource != none)
    {
        self->report("slag::rendergraph: PassBuilder::depthStencil: pass \"%s\": %s", entry.name,
                     entry.queue != CommandQueueType::Graphics ? "attachments are for graphics passes"
                                                               : "only one depth-stencil attachment");
        return;
    }
    entry.depth = {texture.index, loadOp, storeOp, clearValue, mip, baseLayer, layerCount};
    entry.depthReadOnly = readOnly;
    self->addUse(pass, texture.index, true, readOnly ? UseKind::DepthRead : UseKind::DepthWrite, Usage::ShaderRead,
                 {.baseMip = mip, .mipCount = 1, .baseLayer = baseLayer, .layerCount = layerCount},
                 "PassBuilder::depthStencil");
}

Texture* PassContext::texture(RenderGraphTexture texture) const
{
    const RenderGraphImpl* self = impl(graph);
    return texture.index < self->resources.size() ? self->resources[texture.index].textureObject : nullptr;
}

Buffer* PassContext::buffer(RenderGraphBuffer buffer) const
{
    const RenderGraphImpl* self = impl(graph);
    return buffer.index < self->resources.size() ? self->resources[buffer.index].bufferObject : nullptr;
}

uint32_t PassContext::frameSlot() const
{
    return impl(graph)->slot;
}

// --- RenderGraph ----------------------------------------------------------------

Result<void> RenderGraph::beginFrame()
{
    RenderGraphImpl* self = impl(this);
    self->slot = static_cast<uint32_t>(self->frameNumber++ % self->framesInFlight);
    FrameSlot& frame = self->slots[self->slot];
    for (size_t q = 0; q < 3; ++q)
    {
        if (frame.waitValues[q] != 0)
        {
            Result<void> waited = self->semaphores[q]->wait(frame.waitValues[q], UINT64_MAX);
            if (!waited)
            {
                return waited;
            }
        }
        for (uint32_t i = 0; i < frame.usedCommandBuffers[q]; ++i)
        {
            Result<void> reset = frame.commandBuffers[q][i]->reset();
            if (!reset)
            {
                return reset;
            }
        }
        frame.usedCommandBuffers[q] = 0;
    }
    if (self->pendingPass)
    {
        Result<void> continued = self->continueDefragmentation();
        if (!continued)
        {
            return continued;
        }
    }
    self->releaseDefragmentRequest();
    self->defragmentMoves.clear();
    self->destroyPassData();
    self->resources.clear();
    self->declaredUses.clear();
    self->frameOpen = true;
    self->compiled = false;
    self->errors = false;
    self->outOfMemory = false;
    self->epilogueCommandBuffer = nullptr;
    return {};
}

RenderGraphTexture RenderGraph::importTexture(Texture* texture, const ImportedTextureDesc& desc)
{
    RenderGraphImpl* self = impl(this);
    if (!self->declaring("RenderGraph::importTexture"))
    {
        return {};
    }
    if (!texture || desc.finalLayout == TextureLayout::Undefined)
    {
        self->report("slag::rendergraph: RenderGraph::importTexture: %s",
                     !texture ? "the texture is null" : "the final layout can't be Undefined");
        return {};
    }
    ResourceEntry entry;
    entry.texture = true;
    entry.textureObject = texture;
    entry.view = desc.attachmentView;
    entry.import = desc;
    return {self->addResource(std::move(entry))};
}

RenderGraphBuffer RenderGraph::importBuffer(Buffer* buffer)
{
    RenderGraphImpl* self = impl(this);
    if (!self->declaring("RenderGraph::importBuffer"))
    {
        return {};
    }
    if (!buffer)
    {
        self->report("slag::rendergraph: RenderGraph::importBuffer: the buffer is null");
        return {};
    }
    ResourceEntry entry;
    entry.texture = false;
    entry.bufferObject = buffer;
    return {self->addResource(std::move(entry))};
}

namespace {

RenderGraphTexture addTransient(RenderGraphImpl* self, const TransientTexture& shape,
                                std::span<const PixelFormat> castableFormats, const ClearValue* clearValue)
{
    if (!self->declaring("RenderGraph::createTransientTexture*"))
    {
        return {};
    }
    if (shape.format == PixelFormat::Undefined || !castableFormats.empty())
    {
        self->report("slag::rendergraph: RenderGraph::createTransientTexture*: %s",
                     shape.format == PixelFormat::Undefined ? "the format is Undefined"
                                                            : "transient textures have no castable formats");
        return {};
    }
    ResourceEntry entry;
    entry.texture = true;
    entry.transient = true;
    entry.shape = shape;
    entry.hasClearValue = clearValue != nullptr;
    entry.clearValue = clearValue ? *clearValue : ClearValue{};
    return {self->addResource(std::move(entry))};
}

} // namespace

RenderGraphTexture RenderGraph::createTransientTexture2D(const Texture2DDesc& desc)
{
    return addTransient(impl(this),
                        {.kind = TransientKind::Texture2D,
                         .format = desc.format,
                         .width = desc.width,
                         .height = desc.height,
                         .mipCount = desc.mipCount,
                         .sampleCount = desc.sampleCount,
                         .usage = desc.usage,
                         .debugName = desc.debugName},
                        desc.castableFormats, desc.clearValue);
}

RenderGraphTexture RenderGraph::createTransientTexture2DArray(const Texture2DArrayDesc& desc)
{
    return addTransient(impl(this),
                        {.kind = TransientKind::Texture2DArray,
                         .format = desc.format,
                         .width = desc.width,
                         .height = desc.height,
                         .count = desc.layerCount,
                         .mipCount = desc.mipCount,
                         .sampleCount = desc.sampleCount,
                         .usage = desc.usage,
                         .debugName = desc.debugName},
                        desc.castableFormats, desc.clearValue);
}

RenderGraphTexture RenderGraph::createTransientTextureCube(const TextureCubeDesc& desc)
{
    return addTransient(impl(this),
                        {.kind = TransientKind::Cube,
                         .format = desc.format,
                         .width = desc.size,
                         .height = desc.size,
                         .mipCount = desc.mipCount,
                         .usage = desc.usage,
                         .debugName = desc.debugName},
                        desc.castableFormats, desc.clearValue);
}

RenderGraphTexture RenderGraph::createTransientTextureCubeArray(const TextureCubeArrayDesc& desc)
{
    return addTransient(impl(this),
                        {.kind = TransientKind::CubeArray,
                         .format = desc.format,
                         .width = desc.size,
                         .height = desc.size,
                         .count = desc.cubeCount,
                         .mipCount = desc.mipCount,
                         .usage = desc.usage,
                         .debugName = desc.debugName},
                        desc.castableFormats, desc.clearValue);
}

RenderGraphTexture RenderGraph::createTransientTexture3D(const Texture3DDesc& desc)
{
    return addTransient(impl(this),
                        {.kind = TransientKind::Texture3D,
                         .format = desc.format,
                         .width = desc.width,
                         .height = desc.height,
                         .depth = desc.depth,
                         .mipCount = desc.mipCount,
                         .usage = desc.usage,
                         .debugName = desc.debugName},
                        desc.castableFormats, desc.clearValue);
}

RenderGraphBuffer RenderGraph::createTransientBuffer(const BufferDesc& desc)
{
    RenderGraphImpl* self = impl(this);
    if (!self->declaring("RenderGraph::createTransientBuffer"))
    {
        return {};
    }
    if (desc.size == 0)
    {
        self->report("slag::rendergraph: RenderGraph::createTransientBuffer: the size is 0");
        return {};
    }
    ResourceEntry entry;
    entry.texture = false;
    entry.transient = true;
    entry.bufferDesc = desc;
    entry.bufferDesc.placement = {};
    return {self->addResource(std::move(entry))};
}

void* RenderGraph::allocatePassData(size_t size, size_t alignment)
{
    RenderGraphImpl* self = impl(this);
    for (ArenaChunk& chunk : self->chunks)
    {
        // Aligned in address, not in offset: chunks are only 16-byte aligned.
        uintptr_t base = reinterpret_cast<uintptr_t>(chunk.memory);
        size_t start = static_cast<size_t>((base + chunk.used + alignment - 1) / alignment * alignment - base);
        if (start + size <= chunk.size)
        {
            chunk.used = start + size;
            return static_cast<char*>(chunk.memory) + start;
        }
    }
    constexpr size_t chunkSize = 64 * 1024;
    ArenaChunk chunk;
    chunk.size = size + alignment > chunkSize ? size + alignment : chunkSize;
    chunk.memory = self->allocator.allocate(chunk.size, alignment > 16 ? alignment : 16);
    if (!chunk.memory)
    {
        return nullptr;
    }
    if (!self->chunks.pushBack(chunk))
    {
        self->allocator.free(chunk.memory, chunk.size);
        return nullptr;
    }
    ArenaChunk& added = self->chunks[self->chunks.size() - 1];
    uintptr_t base = reinterpret_cast<uintptr_t>(added.memory);
    size_t start = static_cast<size_t>((base + alignment - 1) / alignment * alignment - base);
    added.used = start + size;
    return static_cast<char*>(added.memory) + start;
}

PassBuilder RenderGraph::beginPass(const char* name, CommandQueueType queue, void* data, RecordFn record,
                                   DestroyFn destroy)
{
    RenderGraphImpl* self = impl(this);
    if (!self->declaring("RenderGraph::addPass"))
    {
        if (data)
        {
            destroy(data);
        }
        self->errors = true;
        return PassBuilder(nullptr, 0);
    }
    PassEntry entry;
    entry.name = name ? name : "unnamed";
    entry.queue = queue;
    entry.data = data;
    entry.record = record;
    entry.destroy = destroy;
    entry.beginTextures = Vector<TextureBarrier>(&self->allocator);
    entry.beginBuffers = Vector<BufferBarrier>(&self->allocator);
    entry.endTextures = Vector<TextureBarrier>(&self->allocator);
    entry.endBuffers = Vector<BufferBarrier>(&self->allocator);
    if (!data)
    {
        self->outOfMemory = true;
    }
    if (!self->queues[queueIndex(queue)])
    {
        self->report("slag::rendergraph: RenderGraph::addPass: pass \"%s\": the device has no %s queue", entry.name,
                     queueName(queue));
    }
    if (!self->passes.pushBack(std::move(entry)))
    {
        if (data)
        {
            destroy(data);
        }
        self->outOfMemory = true;
        self->errors = true;
        return PassBuilder(nullptr, 0);
    }
    return PassBuilder(this, static_cast<uint32_t>(self->passes.size() - 1));
}

Result<void> RenderGraph::compile()
{
    RenderGraphImpl* self = impl(this);
    if (!self->declaring("RenderGraph::compile"))
    {
        return Status::Unsupported;
    }
    self->frameOpen = false;
    if (self->outOfMemory)
    {
        return Status::OutOfHostMemory;
    }

    // Resources: subresource counts, and the placement needs of transients.
    self->planResources.clear();
    for (ResourceEntry& entry : self->resources)
    {
        if (entry.texture && entry.transient)
        {
            entry.mipCount = entry.shape.mipCount;
            entry.layerCount = entry.shape.layerCount();
            entry.depth = entry.shape.depth;
        }
        else if (entry.texture)
        {
            entry.mipCount = entry.textureObject->mipCount();
            entry.layerCount = entry.textureObject->layerCount();
            entry.depth = entry.textureObject->depth();
        }
        PlanResource resource;
        resource.texture = entry.texture;
        resource.transient = entry.transient;
        resource.initialLayout = entry.import.initialLayout;
        resource.finalLayout = entry.import.finalLayout;
        resource.mipCount = entry.mipCount;
        resource.layerCount = entry.layerCount;
        if (entry.transient)
        {
            PlacementInfo info =
                entry.texture ? placementOf(self->device, entry.shape) : self->device->placementInfo(entry.bufferDesc);
            resource.size = info.size;
            resource.alignment = info.alignment ? info.alignment : 1;
        }
        if (!self->planResources.pushBack(resource))
        {
            return Status::OutOfHostMemory;
        }
    }

    // Uses, merged per pass, then checked against the queue and the
    // resource's state.
    self->planPasses.clear();
    self->planUses.clear();
    for (uint32_t p = 0; p < self->passes.size(); ++p)
    {
        (void)self->mergeUses(p);
    }
    if (self->outOfMemory)
    {
        return Status::OutOfHostMemory;
    }
    {
        Vector<bool> used(&self->allocator);
        if (!used.reserve(self->resources.size()))
        {
            return Status::OutOfHostMemory;
        }
        for (size_t r = 0; r < self->resources.size(); ++r)
        {
            (void)used.pushBack(false);
        }
        for (uint32_t p = 0; p < self->planPasses.size(); ++p)
        {
            const PlanPass& planPass = self->planPasses[p];
            const PassEntry& pass = self->passes[p];
            for (uint32_t i = 0; i < planPass.useCount; ++i)
            {
                const PlanUse& use = self->planUses[planPass.firstUse + i];
                const ResourceEntry& entry = self->resources[use.resource];
                if (entry.texture && !layoutValidOn(use.layout, pass.queue))
                {
                    self->report("slag::rendergraph: pass \"%s\": %s queues can't use that layout", pass.name,
                                 queueName(pass.queue));
                }
                if (entry.transient && entry.texture)
                {
                    TextureUsage usage = entry.shape.usage;
                    bool target = use.layout == TextureLayout::RenderTarget;
                    bool depth =
                        use.layout == TextureLayout::DepthStencilWrite || use.layout == TextureLayout::DepthStencilRead;
                    bool write = (use.access & BarrierAccess::ShaderWrite) != BarrierAccess::NoAccess;
                    if ((target && (usage & TextureUsage::RenderTarget) == TextureUsage::None) ||
                        (depth && (usage & TextureUsage::DepthStencil) == TextureUsage::None) ||
                        (write && (usage & TextureUsage::ShaderWrite) == TextureUsage::None))
                    {
                        self->report(
                            "slag::rendergraph: pass \"%s\": a transient texture lacks the usage it's used with",
                            pass.name);
                    }
                }
                if (!used[use.resource])
                {
                    used[use.resource] = true;
                    if (entry.transient && !use.write)
                    {
                        self->report(
                            "slag::rendergraph: pass \"%s\": a transient resource is read before anything writes it",
                            pass.name);
                    }
                    bool loads = false;
                    for (uint32_t c = 0; c < pass.colorCount; ++c)
                    {
                        loads =
                            loads || (pass.colors[c].resource == use.resource && pass.colors[c].loadOp == LoadOp::Load);
                    }
                    loads = loads || (pass.depth.resource == use.resource && pass.depth.loadOp == LoadOp::Load);
                    if (entry.transient && loads)
                    {
                        self->report(
                            "slag::rendergraph: pass \"%s\": a transient attachment's first use loads it; clear or "
                            "discard it",
                            pass.name);
                    }
                    if (entry.texture && pass.queue == CommandQueueType::Copy &&
                        (entry.transient || entry.import.initialLayout != TextureLayout::Common))
                    {
                        self->report(
                            "slag::rendergraph: pass \"%s\": a texture first used on a copy queue must be imported "
                            "in the Common layout",
                            pass.name);
                    }
                    else if (entry.texture && !entry.transient && entry.import.initialLayout != use.layout &&
                             entry.import.initialLayout != TextureLayout::Undefined &&
                             !layoutValidOn(entry.import.initialLayout, pass.queue))
                    {
                        self->report(
                            "slag::rendergraph: pass \"%s\": %s queues can't change a texture from its initial "
                            "layout; use it on the graphics queue first",
                            pass.name, queueName(pass.queue));
                    }
                }
            }
            for (uint32_t c = 0; c < pass.colorCount; ++c)
            {
                if (pass.colors[c].resource < self->resources.size() &&
                    !self->resources[pass.colors[c].resource].view &&
                    !self->resources[pass.colors[c].resource].transient)
                {
                    self->report("slag::rendergraph: pass \"%s\": an imported render target needs an attachment view",
                                 pass.name);
                }
            }
            if (pass.depth.resource != none && pass.depth.resource < self->resources.size() &&
                !self->resources[pass.depth.resource].view && !self->resources[pass.depth.resource].transient)
            {
                self->report(
                    "slag::rendergraph: pass \"%s\": an imported depth-stencil target needs an attachment view",
                    pass.name);
            }
        }
    }
    if (self->errors)
    {
        return Status::Unsupported;
    }

    self->plan = Plan(&self->allocator);
    Status status = buildPlan(&self->allocator, {self->planResources.data(), self->planResources.size()},
                              {self->planPasses.data(), self->planPasses.size()},
                              {self->planUses.data(), self->planUses.size()}, self->plan);
    if (status != Status::Ok)
    {
        return status;
    }
    status = self->placeTransients();
    if (status != Status::Ok)
    {
        return status;
    }

    // Command buffers and Slag barriers per pass.
    for (uint32_t p = 0; p < self->passes.size(); ++p)
    {
        PassEntry& pass = self->passes[p];
        pass.recorded = false;
        pass.commandBuffer = self->takeCommandBuffer(pass.queue);
        if (!pass.commandBuffer || !self->convert(self->plan.passes[p].begin, pass.beginTextures, pass.beginBuffers) ||
            !self->convert(self->plan.passes[p].end, pass.endTextures, pass.endBuffers))
        {
            return Status::OutOfHostMemory;
        }
        Attachment* attachments[limits::maxColorTargets + 1];
        uint32_t attachmentCount = 0;
        for (uint32_t c = 0; c < pass.colorCount; ++c)
        {
            attachments[attachmentCount++] = &pass.colors[c];
        }
        if (pass.depth.resource != none)
        {
            attachments[attachmentCount++] = &pass.depth;
        }
        for (uint32_t a = 0; a < attachmentCount; ++a)
        {
            Result<AttachmentView*> view = self->attachmentView(*attachments[a]);
            if (!view)
            {
                return view.status();
            }
            attachments[a]->view = view.value();
        }
    }
    Vector<BufferBarrier> noBuffers(&self->allocator);
    if (!self->plan.epilogue.empty())
    {
        self->epilogueCommandBuffer = self->takeCommandBuffer(CommandQueueType::Graphics);
        if (!self->epilogueCommandBuffer || !self->convert(self->plan.epilogue, self->epilogueBarriers, noBuffers))
        {
            return Status::OutOfHostMemory;
        }
    }

    // Signal values, in submit order, and the submit that first uses each
    // acquired swapchain image.
    self->batchValues.clear();
    self->batchSwapchains.clear();
    for (size_t b = 0; b < self->plan.batches.size(); ++b)
    {
        if (!self->batchValues.pushBack(0) || !self->batchSwapchains.pushBack(none))
        {
            return Status::OutOfHostMemory;
        }
    }
    uint64_t values[3] = {self->semaphoreValues[0], self->semaphoreValues[1], self->semaphoreValues[2]};
    for (uint32_t b : self->plan.submitOrder)
    {
        self->batchValues[b] = ++values[queueIndex(self->plan.batches[b].queue)];
    }
    uint32_t epilogueBatch = none;
    for (uint32_t b = 0; b < self->plan.batches.size(); ++b)
    {
        epilogueBatch = self->plan.batches[b].epilogue ? b : epilogueBatch;
    }
    for (uint32_t r = 0; r < self->resources.size(); ++r)
    {
        const ResourceEntry& entry = self->resources[r];
        if (!entry.import.swapchain)
        {
            continue;
        }
        uint32_t batch =
            self->plan.firstPass[r] != none ? self->plan.passBatch[self->plan.firstPass[r]] : epilogueBatch;
        if (batch == none)
        {
            self->report("slag::rendergraph: an acquired swapchain image is never used");
            return Status::Unsupported;
        }
        // Only one per batch is common; more are chained by resource index.
        self->batchSwapchains[batch] = self->batchSwapchains[batch] == none ? r : self->batchSwapchains[batch];
    }
    self->compiled = true;
    return {};
}

uint32_t RenderGraph::passCount() const
{
    return static_cast<uint32_t>(impl(this)->passes.size());
}

Result<void> RenderGraph::recordPass(uint32_t index)
{
    RenderGraphImpl* self = impl(this);
    if (!self->compiled || index >= self->passes.size() || self->passes[index].recorded)
    {
        self->report("slag::rendergraph: RenderGraph::recordPass: %s",
                     !self->compiled ? "compile first" : "no such pass, or it is already recorded");
        return Status::Unsupported;
    }
    PassEntry& pass = self->passes[index];
    CommandBuffer* commandBuffer = pass.commandBuffer;
    Result<void> begun = commandBuffer->begin();
    if (!begun)
    {
        return begun;
    }
    commandBuffer->beginLabel(pass.name);
    if (!pass.beginTextures.empty() || !pass.beginBuffers.empty())
    {
        commandBuffer->barrier({}, {pass.beginBuffers.data(), pass.beginBuffers.size()},
                               {pass.beginTextures.data(), pass.beginTextures.size()});
    }
    bool rendering = pass.colorCount > 0 || pass.depth.resource != none;
    if (rendering)
    {
        ColorAttachment colors[limits::maxColorTargets];
        for (uint32_t c = 0; c < pass.colorCount; ++c)
        {
            colors[c] = {.view = pass.colors[c].view,
                         .loadOp = pass.colors[c].loadOp,
                         .storeOp = pass.colors[c].storeOp,
                         .clearValue = pass.colors[c].clearValue};
        }
        DepthStencilAttachment depth;
        if (pass.depth.resource != none)
        {
            depth = {.view = pass.depth.view,
                     .depthLoadOp = pass.depth.loadOp,
                     .depthStoreOp = pass.depth.storeOp,
                     .stencilLoadOp = pass.depth.loadOp,
                     .stencilStoreOp = pass.depth.storeOp,
                     .clearValue = pass.depth.clearValue,
                     .readOnlyDepth = pass.depthReadOnly,
                     .readOnlyStencil = pass.depthReadOnly};
        }
        commandBuffer->beginRendering({.colorAttachments = {colors, pass.colorCount},
                                       .depthStencilAttachment = pass.depth.resource != none ? &depth : nullptr});
    }
    PassContext context(this);
    pass.record(pass.data, context, commandBuffer);
    if (rendering)
    {
        commandBuffer->endRendering();
    }
    if (!pass.endTextures.empty() || !pass.endBuffers.empty())
    {
        commandBuffer->barrier({}, {pass.endBuffers.data(), pass.endBuffers.size()},
                               {pass.endTextures.data(), pass.endTextures.size()});
    }
    commandBuffer->endLabel();
    pass.recorded = true;
    return commandBuffer->end();
}

Result<void> RenderGraph::execute()
{
    RenderGraphImpl* self = impl(this);
    if (!self->compiled)
    {
        self->report("slag::rendergraph: RenderGraph::execute: compile first");
        return Status::Unsupported;
    }
    for (const PassEntry& pass : self->passes)
    {
        if (!pass.recorded)
        {
            self->report("slag::rendergraph: RenderGraph::execute: pass \"%s\" isn't recorded", pass.name);
            return Status::Unsupported;
        }
    }
    if (self->epilogueCommandBuffer)
    {
        Result<void> begun = self->epilogueCommandBuffer->begin();
        if (!begun)
        {
            return begun;
        }
        self->epilogueCommandBuffer->barrier({}, {}, {self->epilogueBarriers.data(), self->epilogueBarriers.size()});
        Result<void> ended = self->epilogueCommandBuffer->end();
        if (!ended)
        {
            return ended;
        }
    }
    FrameSlot& frame = self->slots[self->slot];
    for (uint32_t b : self->plan.submitOrder)
    {
        const PlanBatch& batch = self->plan.batches[b];
        size_t q = queueIndex(batch.queue);
        self->submitCommandBuffers.clear();
        self->submitWaits.clear();
        self->submitSwapchains.clear();
        bool ok = true;
        if (batch.epilogue)
        {
            ok = self->submitCommandBuffers.pushBack(self->epilogueCommandBuffer);
        }
        for (uint32_t p : batch.passes)
        {
            ok = ok && self->submitCommandBuffers.pushBack(self->passes[p].commandBuffer);
        }
        for (uint32_t w : batch.waits)
        {
            ok = ok && self->submitWaits.pushBack(
                           {self->semaphores[queueIndex(self->plan.batches[w].queue)], self->batchValues[w]});
        }
        if (self->batchSwapchains[b] != none)
        {
            for (uint32_t r = self->batchSwapchains[b]; r < self->resources.size(); ++r)
            {
                const ResourceEntry& entry = self->resources[r];
                if (!entry.import.swapchain)
                {
                    continue;
                }
                uint32_t first = self->plan.firstPass[r];
                bool here = first != none ? self->plan.passBatch[first] == b : batch.epilogue;
                ok = ok && (!here || self->submitSwapchains.pushBack(entry.import.swapchain));
            }
        }
        if (!ok)
        {
            return Status::OutOfHostMemory;
        }
        SemaphoreValue signal = {self->semaphores[q], self->batchValues[b]};
        Result<void> submitted = self->queues[q]->submit(
            {.waits = {self->submitWaits.data(), self->submitWaits.size()},
             .commandBuffers = {self->submitCommandBuffers.data(), self->submitCommandBuffers.size()},
             .signals = {&signal, 1},
             .acquiredSwapchains = {self->submitSwapchains.data(), self->submitSwapchains.size()}});
        if (!submitted)
        {
            return submitted;
        }
        self->semaphoreValues[q] = self->batchValues[b];
        frame.waitValues[q] = self->batchValues[b];
    }
    self->compiled = false;
    if (self->defragmentRequested)
    {
        Result<DefragmentationPass*> begun = self->device->beginDefragmentation(self->defragmentDesc);
        if (!begun)
        {
            self->releaseDefragmentRequest();
            return begun.status();
        }
        self->pendingPass = begun.value();
        self->pendingRecorded = false;
        Result<void> finished = self->continueDefragmentation();
        if (!finished)
        {
            return finished; // the request stays for beginFrame to continue
        }
        self->releaseDefragmentRequest();
    }
    return {};
}

void RenderGraph::requestDefragmentation(const DefragmentationDesc& desc, void* data, LayoutFn layoutOf,
                                         DestroyFn destroy)
{
    RenderGraphImpl* self = impl(this);
    bool open = self->declaring("RenderGraph::defragment");
    if (open && (self->defragmentRequested || self->pendingPass))
    {
        self->report("slag::rendergraph: RenderGraph::defragment: one defragmentation pass per frame");
    }
    if (!open || self->defragmentRequested || self->pendingPass)
    {
        if (data)
        {
            destroy(data);
        }
        return;
    }
    if (!data)
    {
        self->outOfMemory = true;
        return;
    }
    self->defragmentRequested = true;
    self->defragmentDesc = desc;
    self->layoutData = data;
    self->layoutOf = layoutOf;
    self->layoutDestroy = destroy;
}

std::span<const DefragmentationMove> RenderGraph::movedResources() const
{
    const RenderGraphImpl* self = impl(this);
    return {self->defragmentMoves.data(), self->defragmentMoves.size()};
}

uint64_t RenderGraph::transientMemorySize() const
{
    return impl(this)->plan.transientSize;
}

Result<RenderGraph*> createRenderGraph(const RenderGraphDesc& desc)
{
    if (!desc.device)
    {
        return Status::Unsupported;
    }
    HostAllocator allocator(desc.allocatorCallbacks);
    RenderGraphImpl* self = allocator.create<RenderGraphImpl>(desc);
    if (!self)
    {
        return Status::OutOfHostMemory;
    }
    for (uint32_t i = 0; i < self->framesInFlight; ++i)
    {
        FrameSlot frame;
        frame.textures = Vector<CachedTexture>(&self->allocator);
        frame.views = Vector<CachedView>(&self->allocator);
        frame.buffers = Vector<CachedBuffer>(&self->allocator);
        for (size_t q = 0; q < 3; ++q)
        {
            frame.commandBuffers[q] = Vector<CommandBuffer*>(&self->allocator);
        }
        if (!self->slots.pushBack(std::move(frame)))
        {
            destroy(self);
            return Status::OutOfHostMemory;
        }
    }
    self->epilogueBarriers = Vector<TextureBarrier>(&self->allocator);
    for (size_t q = 0; q < 3; ++q)
    {
        if (!self->queues[q])
        {
            continue;
        }
        Result<Semaphore*> semaphore = desc.device->createSemaphore({.debugName = "render graph"});
        if (!semaphore)
        {
            destroy(self);
            return semaphore.status();
        }
        self->semaphores[q] = semaphore.value();
    }
    return static_cast<RenderGraph*>(self);
}

void destroy(RenderGraph* graph)
{
    if (!graph)
    {
        return;
    }
    RenderGraphImpl* self = impl(graph);
    for (size_t q = 0; q < 3; ++q)
    {
        if (self->semaphores[q] && self->semaphoreValues[q] != 0)
        {
            (void)self->semaphores[q]->wait(self->semaphoreValues[q], UINT64_MAX);
        }
    }
    if (self->pendingPass)
    {
        (void)self->continueDefragmentation();
    }
    self->releaseDefragmentRequest();
    self->destroyPassData();
    for (FrameSlot& frame : self->slots)
    {
        self->destroyCached(frame, false);
        self->device->destroy(frame.block);
        for (size_t q = 0; q < 3; ++q)
        {
            for (CommandBuffer* commandBuffer : frame.commandBuffers[q])
            {
                self->device->destroy(commandBuffer);
            }
        }
    }
    for (size_t q = 0; q < 3; ++q)
    {
        self->device->destroy(self->semaphores[q]);
    }
    for (ArenaChunk& chunk : self->chunks)
    {
        self->allocator.free(chunk.memory, chunk.size);
    }
    HostAllocator allocator = self->allocator;
    allocator.destroy(self);
}

} // namespace slag::rendergraph
