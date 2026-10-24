// A test-only backend that creates plain Slag objects and does nothing else,
// so every validation check can be triggered without a GPU (M7). Linked
// only into slag_validation_tests, with Slag's common code built around it.

#include "NullBackend.h"

#include "common/Logging.h"

#include <cstring>

namespace slag {

namespace {

const BackendInfo* nullInfo();

} // namespace

const BackendInfo* slagBackend_null()
{
    return nullInfo();
}

} // namespace slag

namespace slag::null {

namespace {

template <class T>
T* make(GraphicsDeviceCommon* device)
{
    T* object = device->instance->allocator.create<T>();
    if (object)
    {
        object->device = device;
        object->functions = device->functions;
    }
    return object;
}

} // namespace

Result<InstanceCommon*> createInstance(const InstanceDesc& desc, const HostAllocator& allocator)
{
    auto* instance = allocator.create<InstanceCommon>();
    if (!instance)
    {
        return Status::OutOfHostMemory;
    }
    initInstanceCommon(*instance, desc, *slagBackend_null(), allocator);
    // One device that supports everything; tests narrow it through the
    // internal structs where a check depends on support.
    auto* device = instance->allocator.create<GraphicsDeviceImpl>();
    if (!device || !instance->devices.pushBack(device))
    {
        instance->allocator.destroy(device);
        instance->allocator.destroy(instance);
        return Status::OutOfHostMemory;
    }
    device->instance = instance;
    device->functions = instance->functions;
    std::strcpy(device->info.name, "null device");
    device->info.dedicatedMemoryBytes = 8ull << 30;
    device->info.sharedMemoryBytes = 16ull << 30;
    GraphicsDeviceCapabilities& capabilities = device->capabilities;
    capabilities.unifiedMemory = false;
    capabilities.gpuUploadMemory = true;
    capabilities.maxBufferSize = 1ull << 40;
    capabilities.samplerReduction = true;
    capabilities.conservativeRasterization = true;
    capabilities.depthBounds = true;
    capabilities.frameLatency = true;
    capabilities.copyQueueTimestamps = true;
    capabilities.timestampCalibration = true;
    for (FormatSupport& support : device->formatSupport)
    {
        support = static_cast<FormatSupport>(~0u);
    }
    for (Vector<CommandQueue*>& queues : device->queues)
    {
        queues = Vector<CommandQueue*>(&instance->allocator);
    }
    return instance;
}

void destroyInstance(InstanceCommon* instance)
{
    for (GraphicsDevice* device : instance->devices)
    {
        instance->allocator.destroy(static_cast<GraphicsDeviceImpl*>(device));
    }
    HostAllocator allocator = instance->allocator;
    allocator.destroy(instance);
}

Result<void> initializeDevice(GraphicsDeviceCommon* device, const GraphicsDeviceInitDesc& desc)
{
    const uint32_t counts[3] = {desc.graphicsQueueCount, desc.computeQueueCount, desc.copyQueueCount};
    for (uint32_t type = 0; type < 3; ++type)
    {
        for (uint32_t i = 0; i < counts[type]; ++i)
        {
            auto* queue = make<CommandQueueCommon>(device);
            if (!queue || !device->queues[type].pushBack(queue))
            {
                return Status::OutOfHostMemory;
            }
            queue->type = static_cast<CommandQueueType>(type);
        }
    }
    return {};
}

void shutdownDevice(GraphicsDeviceCommon* device)
{
    for (Vector<CommandQueue*>& queues : device->queues)
    {
        for (CommandQueue* queue : queues)
        {
            device->instance->allocator.destroy(static_cast<CommandQueueCommon*>(queue));
        }
        queues.clear();
    }
}

MemoryBudget memoryBudget(const GraphicsDeviceCommon*)
{
    return {};
}

Result<void> deviceWaitIdle(GraphicsDeviceCommon*)
{
    return {};
}

Result<void> queueWaitIdle(CommandQueueCommon*)
{
    return {};
}

void swapBufferNative(BufferCommon*, BufferCommon*) {}
void swapTextureNative(TextureCommon*, TextureCommon*) {}
void swapAttachmentViewNative(AttachmentViewCommon*, AttachmentViewCommon*) {}
void cmdRecordMoves(CommandBufferCommon*, const MoveEntry*, size_t) {}
void poisonResourceSlot(GraphicsDeviceCommon*, uint32_t, const SlotDescription&) {}

bool describeDeviceLoss(GraphicsDeviceCommon*, char*, size_t)
{
    return false;
}

Result<QueryPoolCommon*> createQueryPool(GraphicsDeviceCommon* device, const QueryPoolDesc&)
{
    QueryPoolCommon* pool = make<QueryPoolCommon>(device);
    return pool ? Result<QueryPoolCommon*>(pool) : Status::OutOfHostMemory;
}

void destroyQueryPool(GraphicsDeviceCommon* device, QueryPoolCommon* pool)
{
    destroyObject(device->instance, pool);
}

void cmdBeginQuery(CommandBufferCommon*, QueryPoolCommon*, uint32_t) {}
void cmdEndQuery(CommandBufferCommon*, QueryPoolCommon*, uint32_t) {}
void cmdWriteTimestamp(CommandBufferCommon*, QueryPoolCommon*, uint32_t) {}
void cmdResolveQueries(CommandBufferCommon*, QueryPoolCommon*, uint32_t, uint32_t, BufferCommon*, uint64_t) {}

uint64_t timestampFrequency(const CommandQueueCommon*)
{
    return 1'000'000'000;
}

Result<TimestampCalibration> calibrateTimestamps(const CommandQueueCommon*)
{
    return TimestampCalibration{};
}

// Work completes at submit, so semaphores hold the values signaled so far.
Result<SemaphoreCommon*> createSemaphore(GraphicsDeviceCommon* device, const SemaphoreDesc& desc)
{
    auto* semaphore = make<NullSemaphore>(device);
    if (!semaphore)
    {
        return Status::OutOfHostMemory;
    }
    semaphore->value = desc.initialValue;
    return static_cast<SemaphoreCommon*>(semaphore);
}

void destroySemaphore(GraphicsDeviceCommon* device, SemaphoreCommon* semaphore)
{
    destroyObject(device->instance, static_cast<NullSemaphore*>(semaphore));
}

uint64_t semaphoreValue(const SemaphoreCommon* semaphore)
{
    return static_cast<const NullSemaphore*>(semaphore)->value;
}

Result<void> signalSemaphore(SemaphoreCommon* semaphore, uint64_t value)
{
    static_cast<NullSemaphore*>(semaphore)->value = value;
    return {};
}

Result<void> waitSemaphores(GraphicsDeviceCommon*, std::span<const SemaphoreValue> values, WaitMode mode, uint64_t)
{
    size_t reached = 0;
    for (const SemaphoreValue& value : values)
    {
        reached += static_cast<NullSemaphore*>(value.semaphore)->value >= value.value ? 1 : 0;
    }
    bool done = mode == WaitMode::All ? reached == values.size() : reached > 0;
    return done ? Result<void>() : Result<void>(Status::Timeout);
}

Result<MemoryBlockCommon*> createMemoryBlock(GraphicsDeviceCommon* device, const MemoryBlockDesc& desc)
{
    MemoryBlockCommon* block = make<MemoryBlockCommon>(device);
    if (!block)
    {
        return Status::OutOfHostMemory;
    }
    block->size = desc.size;
    block->kind = desc.kind;
    return block;
}

void destroyMemoryBlock(GraphicsDeviceCommon* device, MemoryBlockCommon* block)
{
    destroyObject(device->instance, block);
}

Result<BufferCommon*> createBuffer(GraphicsDeviceCommon* device, const BufferDesc& desc)
{
    BufferCommon* buffer = make<BufferCommon>(device);
    if (!buffer)
    {
        return Status::OutOfHostMemory;
    }
    buffer->size = desc.size;
    buffer->kind = desc.placement.memoryBlock ? static_cast<MemoryBlockCommon*>(desc.placement.memoryBlock)->kind
                                              : desc.placement.kind;
    buffer->gpuAddress = 0x10000;
    return buffer;
}

void destroyBuffer(GraphicsDeviceCommon* device, BufferCommon* buffer)
{
    destroyObject(device->instance, buffer);
}

// D3D12-like: 64 KB alignment, sizes rounded up to it.
PlacementInfo bufferPlacementInfo(const GraphicsDeviceCommon*, const BufferDesc& desc)
{
    return {(desc.size + 65535) / 65536 * 65536, 65536};
}

Result<TextureCommon*> createTexture(GraphicsDeviceCommon* device, const TextureCreateInfo&)
{
    TextureCommon* texture = make<TextureCommon>(device);
    return texture ? Result<TextureCommon*>(texture) : Status::OutOfHostMemory;
}

void destroyTexture(GraphicsDeviceCommon* device, TextureCommon* texture)
{
    destroyObject(device->instance, texture);
}

PlacementInfo texturePlacementInfo(const GraphicsDeviceCommon*, const TextureCreateInfo& info)
{
    uint64_t size = uint64_t(info.width) * info.height * info.depth * info.layerCount * 16 * info.sampleCount;
    return {(size + 65535) / 65536 * 65536, 65536};
}

Result<AttachmentViewCommon*> createAttachmentView(GraphicsDeviceCommon* device, const AttachmentViewDesc&)
{
    AttachmentViewCommon* view = make<AttachmentViewCommon>(device);
    return view ? Result<AttachmentViewCommon*>(view) : Status::OutOfHostMemory;
}

void destroyAttachmentView(GraphicsDeviceCommon* device, AttachmentViewCommon* view)
{
    destroyObject(device->instance, view);
}

Result<PipelineCommon*> createGraphicsPipeline(GraphicsDeviceCommon* device, const GraphicsPipelineDesc&)
{
    PipelineCommon* pipeline = make<PipelineCommon>(device);
    return pipeline ? Result<PipelineCommon*>(pipeline) : Status::OutOfHostMemory;
}

size_t pipelineCacheData(const GraphicsDeviceCommon*, std::span<std::byte>)
{
    return 0;
}

Result<PipelineCommon*> createComputePipeline(GraphicsDeviceCommon* device, const ComputePipelineDesc&)
{
    PipelineCommon* pipeline = make<PipelineCommon>(device);
    return pipeline ? Result<PipelineCommon*>(pipeline) : Status::OutOfHostMemory;
}

void destroyPipeline(GraphicsDeviceCommon* device, PipelineCommon* pipeline)
{
    destroyObject(device->instance, pipeline);
}

void writeBufferDescriptor(GraphicsDeviceCommon*, uint32_t, const BufferDescriptorInfo&) {}
void writeTextureDescriptor(GraphicsDeviceCommon*, uint32_t, const TextureDescriptorInfo&) {}
void writeSamplerDescriptor(GraphicsDeviceCommon*, uint32_t, const SamplerDescriptor&) {}

Result<CommandBufferCommon*> createCommandBuffer(GraphicsDeviceCommon* device, const CommandBufferDesc&)
{
    CommandBufferCommon* commandBuffer = make<CommandBufferCommon>(device);
    return commandBuffer ? Result<CommandBufferCommon*>(commandBuffer) : Status::OutOfHostMemory;
}

void destroyCommandBuffer(GraphicsDeviceCommon* device, CommandBufferCommon* commandBuffer)
{
    destroyObject(device->instance, commandBuffer);
}

Result<void> beginCommandBuffer(CommandBufferCommon*)
{
    return {};
}

Result<void> endCommandBuffer(CommandBufferCommon*)
{
    return {};
}

Result<void> resetCommandBuffer(CommandBufferCommon*)
{
    return {};
}

void cmdBindPipeline(CommandBufferCommon*, PipelineCommon*) {}
void cmdBeginRendering(CommandBufferCommon*, const RenderingDesc&, uint32_t, uint32_t, uint32_t) {}
void cmdEndRendering(CommandBufferCommon*) {}
void cmdBindVertexBuffers(CommandBufferCommon*, uint32_t, std::span<const VertexBufferBinding>) {}
void cmdBindIndexBuffer(CommandBufferCommon*, BufferCommon*, uint64_t, uint64_t, IndexFormat) {}
void cmdSetViewports(CommandBufferCommon*, std::span<const Viewport>) {}
void cmdSetScissors(CommandBufferCommon*, std::span<const Rect>) {}
void cmdSetBlendConstant(CommandBufferCommon*, const float[4]) {}
void cmdSetStencilReference(CommandBufferCommon*, uint8_t) {}
void cmdSetDepthBounds(CommandBufferCommon*, float, float) {}
void cmdDraw(CommandBufferCommon*, uint32_t, uint32_t, uint32_t, uint32_t) {}
void cmdIndirect(CommandBufferCommon*, IndirectKind, BufferCommon*, uint64_t, uint32_t, BufferCommon*, uint64_t) {}
void cmdDrawIndexed(CommandBufferCommon*, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) {}
void cmdPushConstants(CommandBufferCommon*, const void*, uint32_t, uint32_t) {}
void cmdSetConstantBuffer(CommandBufferCommon*, uint64_t, uint32_t) {}
void cmdDispatch(CommandBufferCommon*, uint32_t, uint32_t, uint32_t) {}
void cmdBarrier(CommandBufferCommon*, std::span<const GlobalBarrier>, std::span<const BufferBarrier>,
                std::span<const TextureBarrier>)
{}
void cmdCopyBuffer(CommandBufferCommon*, BufferCommon*, uint64_t, BufferCommon*, uint64_t, uint64_t) {}
void cmdBeginLabel(CommandBufferCommon*, const char*, const float*) {}
void cmdEndLabel(CommandBufferCommon*) {}
void cmdInsertLabel(CommandBufferCommon*, const char*, const float*) {}
void cmdResolveTexture(CommandBufferCommon*, TextureCommon*, uint32_t, uint32_t, TextureCommon*, uint32_t) {}
void cmdCopyTexture(CommandBufferCommon*, const TextureLocation&, const TextureLocation&, uint32_t, uint32_t, uint32_t)
{}
void cmdCopyBufferTexture(CommandBufferCommon*, bool, const TextureLocation&, const BufferLocation&, uint32_t, uint32_t,
                          uint32_t)
{}

Result<void> submit(CommandQueueCommon*, const SubmitDesc& desc)
{
    for (const SemaphoreValue& signal : desc.signals)
    {
        static_cast<NullSemaphore*>(signal.semaphore)->value = signal.value;
    }
    return {};
}

// Three B8G8R8A8Unorm images with RenderTarget usage.
Result<SwapchainCommon*> createSwapchain(GraphicsDeviceCommon* device, const SwapchainDesc& desc)
{
    SwapchainCommon* swapchain = make<SwapchainCommon>(device);
    if (!swapchain)
    {
        return Status::OutOfHostMemory;
    }
    swapchain->textures = Vector<TextureCommon*>(&device->instance->allocator);
    swapchain->views = Vector<AttachmentView*>(&device->instance->allocator);
    swapchain->srgbViews = Vector<AttachmentView*>(&device->instance->allocator);
    for (uint32_t i = 0; i < 3; ++i)
    {
        TextureCommon* texture = make<TextureCommon>(device);
        if (!texture || !swapchain->textures.pushBack(texture))
        {
            return Status::OutOfHostMemory;
        }
        texture->format = desc.format;
        texture->width = desc.width;
        texture->height = desc.height;
        texture->usage = TextureUsage::RenderTarget;
        texture->castableFormats[0] = desc.format == PixelFormat::B8G8R8A8Unorm   ? PixelFormat::B8G8R8A8UnormSrgb
                                      : desc.format == PixelFormat::R8G8B8A8Unorm ? PixelFormat::R8G8B8A8UnormSrgb
                                                                                  : desc.format;
        texture->castableCount = 1;
    }
    const PresentMode modes[] = {PresentMode::Fifo, PresentMode::Mailbox, PresentMode::Immediate};
    for (PresentMode mode : modes)
    {
        swapchain->switchableModes[swapchain->switchableCount++] = mode;
    }
    return swapchain;
}

void destroySwapchain(GraphicsDeviceCommon* device, SwapchainCommon* swapchain)
{
    for (TextureCommon* texture : swapchain->textures)
    {
        destroyObject(device->instance, texture);
    }
    destroyObject(device->instance, swapchain);
}

Result<void> reconfigureSwapchain(SwapchainCommon*, const SwapchainReconfigureDesc&)
{
    return {};
}

Result<uint32_t> acquireImage(SwapchainCommon*)
{
    return 0u;
}

Result<void> presentImage(CommandQueueCommon*, SwapchainCommon*, PresentMode)
{
    return {};
}

uint32_t swapchainFormats(const GraphicsDeviceCommon*, const NativeWindow&, std::span<SwapchainFormat> formats)
{
    if (!formats.empty())
    {
        formats[0] = {PixelFormat::B8G8R8A8Unorm, ColorSpace::SrgbNonlinear};
    }
    return 1;
}

uint32_t presentModes(const GraphicsDeviceCommon*, const NativeWindow&, std::span<PresentMode> modes)
{
    if (!modes.empty())
    {
        modes[0] = PresentMode::Fifo;
    }
    return 1;
}

void setHdrMetadata(SwapchainCommon*, const HdrMetadata&) {}
void setMaxFrameLatency(SwapchainCommon*, uint32_t) {}

Result<void> waitForNextFrame(SwapchainCommon*, uint64_t)
{
    return {};
}

} // namespace slag::null

namespace slag {

namespace {

const BackendInfo* nullInfo()
{
    static const BackendFunctions functions = {
#define SLAG_BACKEND_FN(ReturnType, name, parameters) &null::name,
#include "common/BackendFunctions.inl"
#undef SLAG_BACKEND_FN
    };
    static const BackendInfo info = {nullBackend, "null", &functions};
    return &info;
}

} // namespace

} // namespace slag
