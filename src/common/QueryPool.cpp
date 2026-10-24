#include "common/Logging.h"
#include "common/Objects.h"

#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/QueryPool.h>

namespace slag {

Result<QueryPool*> GraphicsDevice::createQueryPool(const QueryPoolDesc& desc)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    [[maybe_unused]] const char* function = "GraphicsDevice::createQueryPool";
    SLAG_VALIDATE(self->instance, self->initialized, function, "device \"%s\" is not initialized", self->info.name);
    SLAG_VALIDATE(self->instance, desc.count > 0, function, "count is 0");
    SLAG_VALIDATE(self->instance, desc.type == QueryType::Timestamp, function,
                  "occlusion queries arrive in milestone M7");
    SLAG_VALIDATE(self->instance, desc.queueType != CommandQueueType::Copy || self->capabilities.copyQueueTimestamps,
                  function, "copy-queue timestamps are not supported on \"%s\"", self->info.name);
    if (desc.type != QueryType::Timestamp)
    {
        return Status::Unsupported;
    }
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    Result<QueryPoolCommon*> pool = SLAG_DISPATCH(self, createQueryPool)(self, desc);
    if (!pool)
    {
        return pool.status();
    }
    pool.value()->userData = desc.userData;
    pool.value()->type = desc.type;
    pool.value()->count = desc.count;
    pool.value()->queueType = desc.queueType;
    return static_cast<QueryPool*>(pool.value());
}

void GraphicsDevice::destroy(QueryPool* pool)
{
    if (!pool)
    {
        return;
    }
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    auto* object = static_cast<QueryPoolCommon*>(pool);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the query pool belongs to another device");
    SLAG_DISPATCH(self, destroyQueryPool)(self, object);
}

uint64_t QueryPool::userData() const
{
    return static_cast<const QueryPoolCommon*>(this)->userData;
}

void QueryPool::setUserData(uint64_t value)
{
    static_cast<QueryPoolCommon*>(this)->userData = value;
}

void CommandBuffer::writeTimestamp(QueryPool* pool, uint32_t index)
{
    auto* self = static_cast<CommandBufferCommon*>(this);
    auto* object = static_cast<QueryPoolCommon*>(pool);
    [[maybe_unused]] InstanceCommon* instance = self->device->instance;
    SLAG_VALIDATE(instance, self->state == CommandBufferState::Recording, "CommandBuffer::writeTimestamp",
                  "not recording");
    SLAG_VALIDATE(instance, object && index < object->count, "CommandBuffer::writeTimestamp",
                  "the pool is null or index %u is out of range", index);
    SLAG_VALIDATE(instance,
                  !object ||
                      (object->queueType == CommandQueueType::Copy) == (self->queueType == CommandQueueType::Copy),
                  "CommandBuffer::writeTimestamp", "copy-queue pools are only used on copy queues, and the reverse");
    SLAG_DISPATCH(self, cmdWriteTimestamp)(self, object, index);
}

void CommandBuffer::resolveQueries(QueryPool* pool, uint32_t first, uint32_t count, Buffer* buffer, uint64_t offset)
{
    auto* self = static_cast<CommandBufferCommon*>(this);
    auto* object = static_cast<QueryPoolCommon*>(pool);
    auto* destination = static_cast<BufferCommon*>(buffer);
    [[maybe_unused]] InstanceCommon* instance = self->device->instance;
    [[maybe_unused]] const char* function = "CommandBuffer::resolveQueries";
    SLAG_VALIDATE(instance, self->state == CommandBufferState::Recording && !self->inRenderPass, function,
                  "not recording, or inside a render pass");
    SLAG_VALIDATE(instance, object && count > 0 && first < object->count && count <= object->count - first, function,
                  "the pool is null or the range is outside it");
    SLAG_VALIDATE(instance,
                  !object ||
                      (object->queueType == CommandQueueType::Copy) == (self->queueType == CommandQueueType::Copy),
                  function, "copy-queue pools are only used on copy queues, and the reverse");
    SLAG_VALIDATE(instance, destination && offset % 8 == 0 && offset + uint64_t(count) * 8 <= destination->size,
                  function, "the buffer is null, the offset isn't a multiple of 8, or the results don't fit");
    SLAG_DISPATCH(self, cmdResolveQueries)(self, object, first, count, destination, offset);
}

uint64_t CommandQueue::timestampFrequency() const
{
    auto* self = static_cast<const CommandQueueCommon*>(this);
    return SLAG_DISPATCH(self, timestampFrequency)(self);
}

Result<TimestampCalibration> CommandQueue::calibrateTimestamps() const
{
    auto* self = static_cast<const CommandQueueCommon*>(this);
    SLAG_VALIDATE(self->device->instance, self->device->capabilities.timestampCalibration,
                  "CommandQueue::calibrateTimestamps", "timestamp calibration is not supported on \"%s\"",
                  self->device->info.name);
    if (!self->device->capabilities.timestampCalibration)
    {
        return Status::Unsupported;
    }
    return SLAG_DISPATCH(self, calibrateTimestamps)(self);
}

} // namespace slag
