#pragma once

// Data every backend shares, between the public classes and the backend
// types (ADR 0065).

#include "common/Backend.h"
#include "common/Formats.h"
#include "common/HostAllocator.h"
#include "common/Vector.h"

#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/CommandQueue.h>
#include <slag/GraphicsDevice.h>
#include <slag/Instance.h>
#include <slag/Pipeline.h>
#include <slag/Semaphore.h>

#include <atomic>
#include <cstdint>

namespace slag {

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
};

struct GraphicsDeviceCommon : GraphicsDevice
{
    InstanceCommon* instance = nullptr;
    const BackendFunctions* functions = nullptr;
    GraphicsDeviceInfo info;
    uint64_t userData = 0;
    bool initialized = false;
    std::atomic<bool> lost = false;
    uint32_t resourceHeapSize = 0; // user-visible slots, while initialized
    uint32_t samplerHeapSize = 0;
    // Filled by the backend when listing the device (ADR 0022).
    FormatSupport formatSupport[pixelFormatCount] = {};
    // Indexed by CommandQueueType; filled by the backend in initialize.
    Vector<CommandQueue*> queues[3];
};

struct CommandQueueCommon : CommandQueue
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    CommandQueueType type = CommandQueueType::Graphics;
};

struct BufferCommon : Buffer
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    uint64_t size = 0;
    MemoryKind kind = MemoryKind::GpuOnly;
    void* mapped = nullptr;
    uint64_t gpuAddress = 0;
};

enum class PipelineKind : uint8_t
{
    Graphics,
    Compute,
};

struct PipelineCommon : Pipeline
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    PipelineKind kind = PipelineKind::Compute;
};

enum class CommandBufferState : uint8_t
{
    Initial,    // created or reset
    Recording,  // between begin and end
    Executable, // ended, ready to submit
};

struct CommandBufferCommon : CommandBuffer
{
    GraphicsDeviceCommon* device = nullptr;
    // A copy of the table pointer, so recording dispatches in one load (ADR 0065).
    const BackendFunctions* functions = nullptr;
    uint64_t userData = 0;
    CommandQueueType queueType = CommandQueueType::Graphics;
    // Validation state.
    CommandBufferState state = CommandBufferState::Initial;
    const PipelineCommon* boundPipeline = nullptr;
};

struct SemaphoreCommon : Semaphore
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

} // namespace slag
