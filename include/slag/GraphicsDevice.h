#pragma once

// GraphicsDevice: info, capabilities, initialization, create/destroy, descriptor
// writes, placement queries, semaphore waits, status, memory budget, and the
// pipeline cache (ADRs 0008, 0015, 0019, 0023, 0029, 0031, 0043, 0053, 0054).

#include <slag/AttachmentView.h>
#include <slag/Barriers.h>
#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/CommandQueue.h>
#include <slag/Descriptors.h>
#include <slag/Limits.h>
#include <slag/Pipeline.h>
#include <slag/PixelFormat.h>
#include <slag/Result.h>
#include <slag/Semaphore.h>
#include <slag/Texture.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace slag {

enum class GraphicsDeviceType : uint8_t
{
    Discrete,
    Integrated,
    Software,
};

struct GraphicsDeviceInfo
{
    char name[256] = {};
    char driverVersion[64] = {}; // for logging only; vendors encode versions differently
    uint32_t vendorId = 0;
    uint32_t deviceId = 0;
    GraphicsDeviceType deviceType = GraphicsDeviceType::Discrete;
    uint64_t dedicatedMemoryBytes = 0;
    uint64_t sharedMemoryBytes = 0;
    uint8_t luid[8] = {}; // all zero where the platform has no LUID (Linux)
};

// Optional features and per-device values (ADR 0015). Grows as features are
// added.
struct GraphicsDeviceCapabilities
{
    bool unifiedMemory = false;    // GPU and CPU share memory; staging copies can be skipped (ADR 0019)
    bool gpuUploadMemory = false;  // MemoryKind::GpuUpload is available (ADR 0019)
    uint64_t maxBufferSize = 0;    // bytes (ADR 0028)
    bool samplerReduction = false; // ReductionMode::Minimum / Maximum (ADR 0033)
};

// This process's usage of one kind of memory against the OS budget, in bytes.
struct MemoryUsage
{
    uint64_t budgetBytes = 0;
    uint64_t usageBytes = 0;
};

// ADR 0019. Dedicated is GPU memory, shared is system memory the GPU can use,
// as in GraphicsDeviceInfo.
struct MemoryBudget
{
    MemoryUsage dedicated;
    MemoryUsage shared;
};

// How copyBufferToTexture / copyTextureToBuffer lay out one mip of one layer
// (all depth slices of a 3D texture) in a buffer. Same on every backend.
struct CopyFootprint
{
    uint64_t size = 0;     // bytes: rowPitch * rowCount * depth
    uint32_t rowPitch = 0; // bytes between rows, a multiple of 256
    uint32_t rowCount = 0; // rows of texels, or of 4x4 blocks when block-compressed
    uint32_t width = 0;    // the mip's size in texels
    uint32_t height = 0;
    uint32_t depth = 0;
};

struct GraphicsDeviceInitDesc
{
    uint32_t resourceHeapSize = defaults::resourceHeapSize; // up to limits::maxResourceHeapSize (ADR 0029)
    uint32_t samplerHeapSize = limits::maxSamplerHeapSize;
    std::span<const std::byte> pipelineCacheData; // from a previous run (ADR 0043)
    uint32_t graphicsQueueCount = 1;              // at least 1 (ADR 0054)
    uint32_t computeQueueCount = 1;
    uint32_t copyQueueCount = 1;
};

class GraphicsDevice
{
public:
    const GraphicsDeviceInfo& info() const;

    // Available before initialize. Thread-safe.
    const GraphicsDeviceCapabilities& capabilities() const;

    // This process's usage against the OS budget. Available before
    // initialize. Thread-safe.
    MemoryBudget memoryBudget() const;

    // Support beyond docs/format-support.md's guaranteed table. Available
    // before initialize. Thread-safe.
    FormatSupport formatSupport(PixelFormat format) const;

    // Makes the device usable. Nothing else may use the device meanwhile.
    Result<void> initialize(const GraphicsDeviceInitDesc& desc);

    // Undoes initialize; every object created from the device must be
    // destroyed first. Nothing else may use the device meanwhile.
    void shutdown();

    // Ok, or DeviceLost once the device has been lost (ADR 0054).
    Status status() const;

    // index < the count requested for that type in initialize.
    CommandQueue* commandQueue(CommandQueueType type, uint32_t index = 0) const;

    Result<MemoryBlock*> createMemoryBlock(const MemoryBlockDesc& desc);
    // Every resource placed in the block must be destroyed first.
    void destroy(MemoryBlock* memoryBlock);

    Result<Buffer*> createBuffer(const BufferDesc& desc);
    void destroy(Buffer* buffer);

    Result<Texture*> createTexture1D(const Texture1DDesc& desc);
    Result<Texture*> createTexture1DArray(const Texture1DArrayDesc& desc);
    Result<Texture*> createTexture2D(const Texture2DDesc& desc);
    Result<Texture*> createTexture2DArray(const Texture2DArrayDesc& desc);
    Result<Texture*> createTexture3D(const Texture3DDesc& desc);
    Result<Texture*> createTextureCube(const TextureCubeDesc& desc);
    Result<Texture*> createTextureCubeArray(const TextureCubeArrayDesc& desc);
    // Every attachment view of the texture must be destroyed first.
    void destroy(Texture* texture);

    Result<AttachmentView*> createAttachmentView(const AttachmentViewDesc& desc);
    void destroy(AttachmentView* view);

    // What a resource needs when placed in a memory block. Creates nothing;
    // the placement field is ignored. Thread-safe.
    PlacementInfo placementInfo(const BufferDesc& desc) const;
    PlacementInfo placementInfo(const Texture1DDesc& desc) const;
    PlacementInfo placementInfo(const Texture1DArrayDesc& desc) const;
    PlacementInfo placementInfo(const Texture2DDesc& desc) const;
    PlacementInfo placementInfo(const Texture2DArrayDesc& desc) const;
    PlacementInfo placementInfo(const Texture3DDesc& desc) const;
    PlacementInfo placementInfo(const TextureCubeDesc& desc) const;
    PlacementInfo placementInfo(const TextureCubeArrayDesc& desc) const;

    // Buffer layout of one mip (ADR 0048). Textures with depth and stencil
    // name a plane. Thread-safe.
    CopyFootprint copyFootprint(const Texture* texture, uint32_t mip, TexturePlane plane = TexturePlane::Color) const;

    Result<Pipeline*> createComputePipeline(const ComputePipelineDesc& desc);
    void destroy(Pipeline* pipeline);

    Result<CommandBuffer*> createCommandBuffer(const CommandBufferDesc& desc);
    void destroy(CommandBuffer* commandBuffer);

    // Writes take effect immediately; a slot the GPU may be reading must not be
    // rewritten (ADR 0031). Different slots may be written concurrently.
    void writeConstantBuffer(uint32_t index, const ConstantBufferDescriptor& descriptor);
    void writeConstantBuffer(std::span<const ConstantBufferDescriptorWrite> writes);
    void writeRawBuffer(uint32_t index, const RawBufferDescriptor& descriptor);
    void writeRawBuffer(std::span<const RawBufferDescriptorWrite> writes);
    void writeStructuredBuffer(uint32_t index, const StructuredBufferDescriptor& descriptor);
    void writeStructuredBuffer(std::span<const StructuredBufferDescriptorWrite> writes);
    void writeTypedBuffer(uint32_t index, const TypedBufferDescriptor& descriptor);
    void writeTypedBuffer(std::span<const TypedBufferDescriptorWrite> writes);
    void writeRWRawBuffer(uint32_t index, const RawBufferDescriptor& descriptor);
    void writeRWRawBuffer(std::span<const RawBufferDescriptorWrite> writes);
    void writeRWStructuredBuffer(uint32_t index, const StructuredBufferDescriptor& descriptor);
    void writeRWStructuredBuffer(std::span<const StructuredBufferDescriptorWrite> writes);
    void writeRWTypedBuffer(uint32_t index, const TypedBufferDescriptor& descriptor);
    void writeRWTypedBuffer(std::span<const TypedBufferDescriptorWrite> writes);
    void writeTexture(uint32_t index, const TextureDescriptor& descriptor);
    void writeTexture(std::span<const TextureDescriptorWrite> writes);
    void writeRWTexture(uint32_t index, const RWTextureDescriptor& descriptor);
    void writeRWTexture(std::span<const RWTextureDescriptorWrite> writes);
    void writeSampler(uint32_t index, const SamplerDescriptor& descriptor);
    void writeSampler(std::span<const SamplerDescriptorWrite> writes);
    // Marks a slot empty. Reading an empty slot is undefined (ADR 0016);
    // validation builds track it (ADR 0031).
    void clearResourceDescriptor(uint32_t index);
    void clearSamplerDescriptor(uint32_t index);

    Result<Semaphore*> createSemaphore(const SemaphoreDesc& desc);
    void destroy(Semaphore* semaphore);

    // CPU wait. Returns Timeout if the timeout passed first. Thread-safe.
    Result<void> waitSemaphores(std::span<const SemaphoreValue> values, WaitMode mode, uint64_t timeoutNs);

    // Waits for all queues. No submits or presents may run meanwhile (ADR 0068).
    Result<void> waitIdle();

    uint64_t userData() const;
    void setUserData(uint64_t value);

    GraphicsDevice(const GraphicsDevice&) = delete;
    GraphicsDevice& operator=(const GraphicsDevice&) = delete;

protected:
    GraphicsDevice() = default;
    ~GraphicsDevice() = default;
};

} // namespace slag
