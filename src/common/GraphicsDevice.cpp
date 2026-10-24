#include "common/Logging.h"
#include "common/Objects.h"
#include "common/SubAllocator.h"

#include <slag/GraphicsDevice.h>

namespace slag {

namespace {

GraphicsDeviceCommon* common(GraphicsDevice* device)
{
    return static_cast<GraphicsDeviceCommon*>(device);
}

const GraphicsDeviceCommon* common(const GraphicsDevice* device)
{
    return static_cast<const GraphicsDeviceCommon*>(device);
}

} // namespace

const GraphicsDeviceInfo& GraphicsDevice::info() const
{
    return common(this)->info;
}

const GraphicsDeviceCapabilities& GraphicsDevice::capabilities() const
{
    return common(this)->capabilities;
}

size_t GraphicsDevice::pipelineCacheData(std::span<std::byte> data) const
{
    const GraphicsDeviceCommon* self = common(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::pipelineCacheData",
                  "device \"%s\" is not initialized", self->info.name);
    return self->initialized ? SLAG_DISPATCH(self, pipelineCacheData)(self, data) : 0;
}

MemoryBudget GraphicsDevice::memoryBudget() const
{
    const GraphicsDeviceCommon* self = common(this);
    return SLAG_DISPATCH(self, memoryBudget)(self);
}

FormatSupport GraphicsDevice::formatSupport(PixelFormat format) const
{
    const GraphicsDeviceCommon* self = common(this);
    size_t index = static_cast<size_t>(format);
    SLAG_VALIDATE(self->instance, index < pixelFormatCount, "GraphicsDevice::formatSupport",
                  "format %zu is not a PixelFormat", index);
    return index < pixelFormatCount ? self->formatSupport[index] : FormatSupport::None;
}

Result<void> GraphicsDevice::initialize(const GraphicsDeviceInitDesc& desc)
{
    GraphicsDeviceCommon* self = common(this);
    SLAG_VALIDATE(self->instance, !self->initialized, "GraphicsDevice::initialize",
                  "device \"%s\" is already initialized", self->info.name);
    SLAG_VALIDATE(self->instance, desc.resourceHeapSize <= limits::maxResourceHeapSize, "GraphicsDevice::initialize",
                  "resourceHeapSize %u exceeds limits::maxResourceHeapSize (%u)", desc.resourceHeapSize,
                  limits::maxResourceHeapSize);
    SLAG_VALIDATE(self->instance, desc.samplerHeapSize <= limits::maxSamplerHeapSize, "GraphicsDevice::initialize",
                  "samplerHeapSize %u exceeds limits::maxSamplerHeapSize (%u)", desc.samplerHeapSize,
                  limits::maxSamplerHeapSize);
    SLAG_VALIDATE(self->instance, desc.graphicsQueueCount >= 1, "GraphicsDevice::initialize",
                  "graphicsQueueCount must be at least 1");
    if (self->initialized)
    {
        return Status::Unsupported; // reported above; initializing again would leak the first
    }

    Result<void> result = SLAG_DISPATCH(self, initializeDevice)(self, desc);
    if (result)
    {
        self->subAllocator = self->instance->allocator.create<SubAllocator>(self);
        if (!self->subAllocator)
        {
            SLAG_DISPATCH(self, shutdownDevice)(self);
            return Status::OutOfHostMemory;
        }
        self->initialized = true;
        self->lost = false;
        self->resourceHeapSize = desc.resourceHeapSize;
        self->samplerHeapSize = desc.samplerHeapSize;
#ifdef SLAG_VALIDATION
        // Validation only, so a failed allocation just turns stale-slot reports off.
        void* records =
            self->instance->allocator.allocate(sizeof(GraphicsDeviceCommon::SlotRecord) * desc.resourceHeapSize,
                                               alignof(GraphicsDeviceCommon::SlotRecord));
        self->slotRecords = static_cast<GraphicsDeviceCommon::SlotRecord*>(records);
        for (uint32_t i = 0; records && i < desc.resourceHeapSize; ++i)
        {
            new (&self->slotRecords[i]) GraphicsDeviceCommon::SlotRecord();
        }
        self->staleSlots = Vector<GraphicsDeviceCommon::StaleSlot>(&self->instance->allocator);
#endif
    }
    return result;
}

void GraphicsDevice::shutdown()
{
    GraphicsDeviceCommon* self = common(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::shutdown", "device \"%s\" is not initialized",
                  self->info.name);
    if (!self->initialized)
    {
        return;
    }
    SLAG_VALIDATE(self->instance, !self->activePass, "GraphicsDevice::shutdown",
                  "a defragmentation pass is still active");
#ifdef SLAG_VALIDATION
    if (self->slotRecords)
    {
        self->instance->allocator.free(self->slotRecords,
                                       sizeof(GraphicsDeviceCommon::SlotRecord) * self->resourceHeapSize);
        self->slotRecords = nullptr;
    }
    self->staleSlots = Vector<GraphicsDeviceCommon::StaleSlot>(&self->instance->allocator); // frees it
#endif
    // Pool blocks go before the device; every resource must already be destroyed.
    self->instance->allocator.destroy(self->subAllocator);
    self->subAllocator = nullptr;
    SLAG_DISPATCH(self, shutdownDevice)(self);
    for (Vector<CommandQueue*>& queues : self->queues)
    {
        queues.clear();
    }
    self->initialized = false;
}

Status GraphicsDevice::status() const
{
    return common(this)->lost ? Status::DeviceLost : Status::Ok;
}

CommandQueue* GraphicsDevice::commandQueue(CommandQueueType type, uint32_t index) const
{
    const GraphicsDeviceCommon* self = common(this);
    const Vector<CommandQueue*>& queues = self->queues[static_cast<size_t>(type)];
    SLAG_VALIDATE(self->instance, index < queues.size(), "GraphicsDevice::commandQueue",
                  "queue index %u is out of range (%zu queues of this type)", index, queues.size());
    return index < queues.size() ? queues[index] : nullptr;
}

Result<Semaphore*> GraphicsDevice::createSemaphore(const SemaphoreDesc& desc)
{
    GraphicsDeviceCommon* self = common(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::createSemaphore",
                  "device \"%s\" is not initialized", self->info.name);
    if (!self->initialized)
    {
        return Status::Unsupported; // reported above; nothing to create it on
    }
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    Result<SemaphoreCommon*> semaphore = SLAG_DISPATCH(self, createSemaphore)(self, desc);
    if (!semaphore)
    {
        return semaphore.status();
    }
    semaphore.value()->userData = desc.userData;
    setDebugName(self->instance, semaphore.value(), desc.debugName);
    return static_cast<Semaphore*>(semaphore.value());
}

void GraphicsDevice::destroy(Semaphore* semaphore)
{
    SLAG_CHECK_LIVE(static_cast<const GraphicsDeviceCommon*>(this)->instance, SemaphoreCommon, semaphore, "Semaphore",
                    "GraphicsDevice::destroy");
#ifdef SLAG_VALIDATION
    if (semaphore && static_cast<const SemaphoreCommon*>(semaphore)->destroyed)
    {
        return; // reported above: destroying it again would free it twice
    }
#endif
    if (!semaphore)
    {
        return;
    }
    GraphicsDeviceCommon* self = common(this);
    auto* object = static_cast<SemaphoreCommon*>(semaphore);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the semaphore belongs to another device");
    if (object->device != self)
    {
        return; // the other device's state isn't this one's to change
    }
    SLAG_DISPATCH(self, destroySemaphore)(self, object);
}

Result<void> GraphicsDevice::waitSemaphores(std::span<const SemaphoreValue> values, WaitMode mode, uint64_t timeoutNs)
{
    GraphicsDeviceCommon* self = common(this);
#ifdef SLAG_VALIDATION
    for (const SemaphoreValue& value : values)
    {
        SLAG_VALIDATE(self->instance, value.semaphore, "GraphicsDevice::waitSemaphores", "a semaphore is null");
        SLAG_CHECK_LIVE(self->instance, SemaphoreCommon, value.semaphore, "Semaphore",
                        "GraphicsDevice::waitSemaphores");
        SLAG_VALIDATE(self->instance,
                      !value.semaphore || static_cast<SemaphoreCommon*>(value.semaphore)->device == self,
                      "GraphicsDevice::waitSemaphores", "a semaphore belongs to another device");
    }
#endif
    if (values.empty())
    {
        return {};
    }
    for (const SemaphoreValue& value : values)
    {
        if (!value.semaphore)
        {
            return Status::Unsupported; // reported above
        }
    }
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    return SLAG_DISPATCH(self, waitSemaphores)(self, values, mode, timeoutNs);
}

Result<void> GraphicsDevice::waitIdle()
{
    GraphicsDeviceCommon* self = common(this);
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    return SLAG_DISPATCH(self, deviceWaitIdle)(self);
}

uint64_t GraphicsDevice::userData() const
{
    return common(this)->userData;
}

void GraphicsDevice::setUserData(uint64_t value)
{
    common(this)->userData = value;
}

Status reportDeviceLost(GraphicsDeviceCommon* device)
{
    bool wasLost = device->lost.exchange(true);
    if (!wasLost)
    {
        logMessage(messageSink(device->instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: device \"%s\" was lost", device->info.name);
        // One message with everything the backend collected (ADR 0014).
        constexpr size_t reportSize = 16384;
        auto* report = static_cast<char*>(
            device->instance->crashDiagnostics ? device->instance->allocator.allocate(reportSize, 1) : nullptr);
        if (report && SLAG_DISPATCH(device, describeDeviceLoss)(device, report, reportSize))
        {
            logText(messageSink(device->instance), MessageSeverity::Error, MessageSource::Backend, report);
        }
        if (report)
        {
            device->instance->allocator.free(report, reportSize);
        }
    }
    return Status::DeviceLost;
}

// --- CommandQueue -----------------------------------------------------------

CommandQueueType CommandQueue::type() const
{
    return static_cast<const CommandQueueCommon*>(this)->type;
}

Result<void> CommandQueue::waitIdle()
{
    auto* self = static_cast<CommandQueueCommon*>(this);
    if (self->device->lost)
    {
        return Status::DeviceLost;
    }
    return SLAG_DISPATCH(self, queueWaitIdle)(self);
}

// --- Semaphore --------------------------------------------------------------

uint64_t Semaphore::value() const
{
    SLAG_CHECK_LIVE_THIS(SemaphoreCommon, "Semaphore", "Semaphore::value");
    auto* self = static_cast<const SemaphoreCommon*>(this);
    return SLAG_DISPATCH(self, semaphoreValue)(self);
}

Result<void> Semaphore::signal(uint64_t value)
{
    SLAG_CHECK_LIVE_THIS(SemaphoreCommon, "Semaphore", "Semaphore::signal");
    auto* self = static_cast<SemaphoreCommon*>(this);
#ifdef SLAG_VALIDATION
    uint64_t current = SLAG_DISPATCH(self, semaphoreValue)(self);
    SLAG_VALIDATE(self->device->instance, value > current, "Semaphore::signal",
                  "values must strictly increase (current %llu, signaled %llu)",
                  static_cast<unsigned long long>(current), static_cast<unsigned long long>(value));
#endif
    if (self->device->lost)
    {
        return Status::DeviceLost;
    }
    return SLAG_DISPATCH(self, signalSemaphore)(self, value);
}

uint64_t Semaphore::userData() const
{
    SLAG_CHECK_LIVE_THIS(SemaphoreCommon, "Semaphore", "Semaphore::userData");
    return static_cast<const SemaphoreCommon*>(this)->userData;
}

void Semaphore::setUserData(uint64_t value)
{
    SLAG_CHECK_LIVE_THIS(SemaphoreCommon, "Semaphore", "Semaphore::setUserData");
    static_cast<SemaphoreCommon*>(this)->userData = value;
}

} // namespace slag
