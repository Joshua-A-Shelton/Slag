#pragma once

// Data every backend shares, between the public classes and the backend
// types (ADR 0065).

#include "common/Backend.h"
#include "common/Formats.h"
#include "common/HostAllocator.h"
#include "common/Logging.h"
#include "common/Vector.h"
#ifdef SLAG_VALIDATION
#include "common/validation/Quarantine.h"
#endif

#include <slag/AttachmentView.h>
#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/CommandQueue.h>
#include <slag/Defragmentation.h>
#include <slag/Descriptors.h>
#include <slag/GraphicsDevice.h>
#include <slag/Instance.h>
#include <slag/MemoryBlock.h>
#include <slag/Pipeline.h>
#include <slag/QueryPool.h>
#include <slag/Semaphore.h>
#include <slag/Swapchain.h>
#include <slag/Texture.h>

#include <atomic>
#include <cstdint>
#include <mutex>

namespace slag {

struct PoolBlock;
class SubAllocator;
struct AttachmentViewCommon;
struct DefragmentationPassCommon;

// Every Slag object's record (ADRs 0017, 0052): its debug name, kept in
// every build, and in validation builds whether it was destroyed.
struct ObjectRecord
{
    char* debugName = nullptr; // a copy; null when unnamed
#ifdef SLAG_VALIDATION
    bool destroyed = false; // set on destroy; the record stays in the quarantine
#endif
};

struct InstanceCommon : Instance
{
    const BackendInfo* backend = nullptr;
    const BackendFunctions* functions = nullptr;
    HostAllocator allocator;
    DebugMessageFn messageCallback = nullptr;
    void* messageUserData = nullptr;
    ValidationFlags validationFlags = ValidationFlags::None;
    ValidationFailureAction failureAction = ValidationFailureAction::Break;
    Vector<GraphicsDevice*> devices;
    uint64_t userData = 0;
#ifdef SLAG_VALIDATION
    Quarantine quarantine;
#endif
};

struct GraphicsDeviceCommon : GraphicsDevice, ObjectRecord
{
    InstanceCommon* instance = nullptr;
    const BackendFunctions* functions = nullptr;
    GraphicsDeviceInfo info;
    GraphicsDeviceCapabilities capabilities; // filled by the backend when listing
    uint64_t userData = 0;
    bool initialized = false;
    std::atomic<bool> lost = false;
    uint32_t resourceHeapSize = 0; // user-visible slots, while initialized
    uint32_t samplerHeapSize = 0;
    SubAllocator* subAllocator = nullptr; // while initialized
    // Filled by the backend when listing the device (ADR 0022).
    FormatSupport formatSupport[pixelFormatCount] = {};
    // Indexed by CommandQueueType; filled by the backend in initialize.
    Vector<CommandQueue*> queues[3];
    DefragmentationPassCommon* activePass = nullptr; // ADR 0062
    std::mutex viewListLock;                         // TextureCommon::views (ADR 0068)
#ifdef SLAG_VALIDATION
    // Which buffer or texture each resource slot was last written with, to
    // report stale slots after defragmentation (ADR 0062).
    struct SlotRecord
    {
        std::atomic<const void*> object = nullptr;
        std::atomic<uint32_t> moveCount = 0; // the object's moveCount when written
    };
    struct StaleSlot
    {
        uint32_t slot;
        const void* object;
        uint32_t moveCount;
    };
    SlotRecord* slotRecords = nullptr; // resourceHeapSize entries, while initialized
    Vector<StaleSlot> staleSlots;      // checked at the next submit
    std::mutex staleSlotLock;
    std::atomic<bool> hasStaleSlots = false; // so submits only lock when there are some
#endif
};

struct CommandQueueCommon : CommandQueue, ObjectRecord
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    CommandQueueType type = CommandQueueType::Graphics;
};

// A resource's place in the sub-allocator (SubAllocator.h); empty when it
// was placed by the user or has dedicated memory.
struct PoolAllocation
{
    PoolBlock* block = nullptr;
    uint32_t node = 0;
};

struct MemoryBlockCommon : MemoryBlock, ObjectRecord
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    uint64_t size = 0;
    MemoryKind kind = MemoryKind::GpuOnly;
    std::atomic<uint32_t> liveResources = 0; // placed resources, for validation
};

struct BufferCommon : Buffer, ObjectRecord
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    uint64_t size = 0;
    MemoryKind kind = MemoryKind::GpuOnly;
    void* mapped = nullptr;
    uint64_t gpuAddress = 0;
    MemoryBlockCommon* memoryBlock = nullptr; // set when placed by the user
    PoolAllocation allocation;                // set when sub-allocated
    // Defragmentation (ADR 0062).
    DefragmentationPassCommon* movePass = nullptr; // set while in an active pass's move list
    uint32_t moveIndex = 0;
    uint32_t moveCount = 0; // times moved
};

enum class TextureType : uint8_t
{
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
};

// Every texture Desc, normalized (ADR 0021). layerCount counts cube faces.
struct TextureCreateInfo
{
    TextureType type = TextureType::Texture2D;
    PixelFormat format = PixelFormat::Undefined;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t layerCount = 1;
    uint32_t mipCount = 1;
    uint32_t sampleCount = 1;
    TextureUsage usage = TextureUsage::None;
    std::span<const PixelFormat> castableFormats;
    const ClearValue* clearValue = nullptr;
    Placement placement;
    const char* debugName = nullptr;
    uint64_t userData = 0;
    // A defragmentation move's new texture: D3D12 creates it in COMMON, so
    // copy queues can write it (ADR 0062).
    bool moveTarget = false;
};

struct TextureCommon : Texture, ObjectRecord
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    TextureType type = TextureType::Texture2D;
    PixelFormat format = PixelFormat::Undefined;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t layerCount = 1;
    uint32_t mipCount = 1;
    uint32_t sampleCount = 1;
    TextureUsage usage = TextureUsage::None;
    MemoryBlockCommon* memoryBlock = nullptr; // set when placed by the user
    PoolAllocation allocation;                // set when sub-allocated
    // Defragmentation (ADR 0062): what recreating the texture needs, and its
    // attachment views (under the device's viewListLock).
    ClearValue clearValue;
    bool hasClearValue = false;
    AttachmentViewCommon* views = nullptr;
    DefragmentationPassCommon* movePass = nullptr;
    uint32_t moveIndex = 0;
    uint32_t moveCount = 0;
    // For validation.
    PixelFormat castableFormats[16] = {};
    uint32_t castableCount = 0;
    std::atomic<uint32_t> liveViews = 0;
};

struct AttachmentViewCommon : AttachmentView, ObjectRecord
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    TextureCommon* texture = nullptr;
    PixelFormat format = PixelFormat::Undefined; // resolved: never Undefined
    uint32_t mip = 0;
    uint32_t baseLayer = 0;
    uint32_t layerCount = 1;
    // In the texture's view list.
    AttachmentViewCommon* previousView = nullptr;
    AttachmentViewCommon* nextView = nullptr;
};

enum class BufferDescriptorKind : uint8_t
{
    Constant,
    Raw,
    Structured,
    Typed,
    RWRaw,
    RWStructured,
    RWTyped,
};

// Every buffer descriptor kind, normalized (ADR 0030). `address` and `size`
// give the range; count, stride, and format are for structured and typed kinds.
struct BufferDescriptorInfo
{
    BufferDescriptorKind kind = BufferDescriptorKind::Raw;
    BufferCommon* buffer = nullptr; // null for constant buffers (address only)
    uint64_t address = 0;
    uint64_t offset = 0; // into buffer
    uint64_t size = 0;
    uint32_t count = 0;
    uint32_t stride = 0;
    PixelFormat format = PixelFormat::Undefined;
};

// Texture and RW texture descriptors, normalized: format and counts resolved.
struct TextureDescriptorInfo
{
    TextureCommon* texture = nullptr;
    bool writable = false;
    TextureViewType viewType = TextureViewType::Texture2D;
    PixelFormat format = PixelFormat::Undefined;
    uint32_t baseMip = 0;
    uint32_t mipCount = 1;
    uint32_t baseLayer = 0;
    uint32_t layerCount = 1;
    TexturePlane plane = TexturePlane::Color;
};

enum class IndirectKind : uint8_t
{
    Draw,
    DrawIndexed,
    Dispatch,
};

enum class PipelineKind : uint8_t
{
    Graphics,
    Compute,
};

struct PipelineCommon : Pipeline, ObjectRecord
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    PipelineKind kind = PipelineKind::Compute;
    // Graphics pipelines, for validation against the render pass.
    PixelFormat colorFormats[8] = {};
    uint32_t colorCount = 0;
    PixelFormat depthStencilFormat = PixelFormat::Undefined;
    uint32_t sampleCount = 1;
    bool writesDepth = false;
    bool writesStencil = false;
    uint32_t vertexSlotCount = 0;
};

enum class CommandBufferState : uint8_t
{
    Initial,    // created or reset
    Recording,  // between begin and end
    Executable, // ended, ready to submit
};

struct CommandBufferCommon : CommandBuffer, ObjectRecord
{
    GraphicsDeviceCommon* device = nullptr;
    // A copy of the table pointer, so recording dispatches in one load (ADR 0065).
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    CommandQueueType queueType = CommandQueueType::Graphics;
    // Validation state.
    CommandBufferState state = CommandBufferState::Initial;
    const PipelineCommon* boundPipeline = nullptr;
    // The open render pass, for validation.
    bool inRenderPass = false;
    PixelFormat passColorFormats[8] = {};
    uint32_t passColorCount = 0;
    PixelFormat passDepthStencilFormat = PixelFormat::Undefined;
    uint32_t passSampleCount = 1;
    bool passReadOnlyDepth = false;
    bool passReadOnlyStencil = false;
    // Split render passes (ADR 0046): the first pass resumes, the last suspends.
    bool resumesPass = false;
    bool suspendsPass = false;
    bool recordedPass = false;
    int32_t labelDepth = 0; // open labels (ADR 0050); below 0 when closing ones a split pass opened
};

struct SwapchainCommon : Swapchain, ObjectRecord
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    CommandQueueCommon* queue = nullptr;
    NativeWindow window;
    uint32_t width = 0;
    uint32_t height = 0;
    PixelFormat format = PixelFormat::B8G8R8A8Unorm;
    ColorSpace colorSpace = ColorSpace::SrgbNonlinear;
    // Filled by the backend at create and reconfigure; common code makes the views.
    Vector<TextureCommon*> textures;
    Vector<AttachmentView*> views;     // one per image, in the swapchain format
    Vector<AttachmentView*> srgbViews; // one per image, the sRGB variant (8-bit formats)
    PresentMode switchableModes[3] = {};
    uint32_t switchableCount = 0;
    // Validation: the acquired, not yet presented image, or none.
    static constexpr uint32_t noImage = UINT32_MAX;
    uint32_t acquiredImage = noImage;
};

struct QueryPoolCommon : QueryPool, ObjectRecord
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    QueryType type = QueryType::Timestamp;
    uint32_t count = 0;
    CommandQueueType queueType = CommandQueueType::Graphics;
};

// One move of a defragmentation pass: the temporary objects hold the new
// native objects until endDefragmentation swaps them in (ADR 0062).
struct MoveEntry
{
    BufferCommon* buffer = nullptr; // the user's object, or null once dropped
    TextureCommon* texture = nullptr;
    BufferCommon* newBuffer = nullptr;
    TextureCommon* newTexture = nullptr;
    PoolAllocation from;
    PoolAllocation to;
};

#ifdef SLAG_VALIDATION
// Records the resource a slot now holds, or null (ADR 0062).
inline void recordSlot(GraphicsDeviceCommon* device, uint32_t index, const void* object, uint32_t moveCount)
{
    if (device->slotRecords && index < device->resourceHeapSize)
    {
        device->slotRecords[index].object.store(object, std::memory_order_relaxed);
        device->slotRecords[index].moveCount.store(moveCount, std::memory_order_relaxed);
    }
}
#endif

struct DefragmentationPassCommon : DefragmentationPass
{
    GraphicsDeviceCommon* device = nullptr;
    MemoryKind kind = MemoryKind::GpuOnly;
    Vector<DefragmentationMove> moves;
    Vector<MoveEntry> entries; // parallel to moves
    bool recorded = false;
};

// Abandons one move: its resource is being destroyed (Defragmentation.cpp).
void dropMove(DefragmentationPassCommon* pass, uint32_t index);

#ifdef SLAG_VALIDATION
// Reports, once, resource slots a defragmentation pass left stale.
void reportStaleSlots(GraphicsDeviceCommon* device);
#endif

struct SemaphoreCommon : Semaphore, ObjectRecord
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
};

// Marks the device lost the first time a backend sees it, and returns
// DeviceLost (ADR 0054).
Status reportDeviceLost(GraphicsDeviceCommon* device);

// Fills the shared fields; backends call this first when creating an instance.
void initInstanceCommon(InstanceCommon& instance, const InstanceDesc& desc, const BackendInfo& backend,
                        const HostAllocator& allocator);

// Debug names (ADR 0017): copied at creation, in every build.
void setDebugName(InstanceCommon* instance, ObjectRecord* record, const char* name);
void releaseDebugName(InstanceCommon* instance, ObjectRecord* record);
// The name for messages: "" when unnamed.
inline const char* debugNameOf(const ObjectRecord* record)
{
    return record && record->debugName ? record->debugName : "";
}

// Frees a destroyed Slag object (T is its exact type). Validation builds keep
// it in the quarantine instead, marked destroyed (ADR 0052).
template <class T>
void destroyObject(InstanceCommon* instance, T* object)
{
#ifdef SLAG_VALIDATION
    object->destroyed = true;
    instance->quarantine.add(instance, object, [](InstanceCommon* owner, void* pointer) {
        T* quarantined = static_cast<T*>(pointer);
        releaseDebugName(owner, quarantined);
        owner->allocator.destroy(quarantined);
    });
#else
    releaseDebugName(instance, object);
    instance->allocator.destroy(object);
#endif
}

#ifdef SLAG_VALIDATION
// Reports use of a destroyed object (a public pointer, converted to its
// Common type); `object` may be null.
#define SLAG_CHECK_LIVE(instance, CommonType, object, typeName, function)                                              \
    SLAG_VALIDATE_LIVE((instance), static_cast<const CommonType*>(object), (typeName), (function))
// The object a method is called on (never null).
#define SLAG_CHECK_LIVE_THIS(CommonType, typeName, function)                                                           \
    SLAG_VALIDATE(static_cast<const CommonType*>(this)->device->instance,                                              \
                  !static_cast<const CommonType*>(this)->destroyed, (function), "use of a destroyed %s \"%s\"",        \
                  (typeName), ::slag::debugNameOf(static_cast<const CommonType*>(this)))
#else
#define SLAG_CHECK_LIVE_THIS(CommonType, typeName, function)                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (false)
#define SLAG_CHECK_LIVE(instance, CommonType, object, typeName, function)                                              \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (false)
#endif

#ifdef SLAG_VALIDATION
// Reports use of a destroyed object; `object` may be null.
#define SLAG_VALIDATE_LIVE(instance, object, typeName, function)                                                       \
    SLAG_VALIDATE((instance), !(object) || !(object)->destroyed, (function), "use of a destroyed %s \"%s\"",           \
                  (typeName), ::slag::debugNameOf(object))
#else
#define SLAG_VALIDATE_LIVE(instance, object, typeName, function)                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (false)
#endif

} // namespace slag
