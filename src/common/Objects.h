#pragma once

// Data every backend shares, between the public classes and the backend
// types (ADR 0065).

#include "common/Backend.h"
#include "common/HostAllocator.h"
#include "common/Vector.h"

#include <slag/CommandQueue.h>
#include <slag/GraphicsDevice.h>
#include <slag/Instance.h>
#include <slag/Semaphore.h>

#include <atomic>
#include <cstdint>

namespace slag {

struct InstanceCommon : Instance
{
    const BackendInfo* backend = nullptr;
    const BackendFunctions* functions = nullptr;
    HostAllocator allocator;
    DebugMessageFn debugCallback = nullptr;
    void* debugUserData = nullptr;
    ValidationFlags validation = ValidationFlags::None;
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
    // Indexed by CommandQueueType; filled by the backend in initialize.
    Vector<CommandQueue*> queues[3];
};

struct CommandQueueCommon : CommandQueue
{
    GraphicsDeviceCommon* device = nullptr;
    const BackendFunctions* functions = nullptr;
    CommandQueueType type = CommandQueueType::Graphics;
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
