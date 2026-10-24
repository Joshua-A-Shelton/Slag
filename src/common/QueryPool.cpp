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
    SLAG_VALIDATE(self->instance, desc.type != QueryType::Occlusion || desc.queueType == CommandQueueType::Graphics,
                  function, "occlusion pools are for graphics queues");
    SLAG_VALIDATE(self->instance,
                  desc.type != QueryType::Timestamp || desc.queueType != CommandQueueType::Copy ||
                      self->capabilities.copyQueueTimestamps,
                  function, "copy-queue timestamps are not supported on \"%s\"", self->info.name);
    if (!self->initialized)
    {
        return Status::Unsupported; // reported above; nothing to create it on
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
    setDebugName(self->instance, pool.value(), desc.debugName);
    pool.value()->type = desc.type;
    pool.value()->count = desc.count;
    pool.value()->queueType = desc.queueType;
    return static_cast<QueryPool*>(pool.value());
}

void GraphicsDevice::destroy(QueryPool* pool)
{
    SLAG_CHECK_LIVE(static_cast<const GraphicsDeviceCommon*>(this)->instance, QueryPoolCommon, pool, "QueryPool",
                    "GraphicsDevice::destroy");
#ifdef SLAG_VALIDATION
    if (pool && static_cast<const QueryPoolCommon*>(pool)->destroyed)
    {
        return; // reported above: destroying it again would free it twice
    }
#endif
    if (!pool)
    {
        return;
    }
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    auto* object = static_cast<QueryPoolCommon*>(pool);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the query pool belongs to another device");
    if (object->device != self)
    {
        return; // the other device's state isn't this one's to change
    }
    SLAG_DISPATCH(self, destroyQueryPool)(self, object);
}

uint64_t QueryPool::userData() const
{
    SLAG_CHECK_LIVE_THIS(QueryPoolCommon, "QueryPool", "QueryPool::userData");
    return static_cast<const QueryPoolCommon*>(this)->userData;
}

void QueryPool::setUserData(uint64_t value)
{
    SLAG_CHECK_LIVE_THIS(QueryPoolCommon, "QueryPool", "QueryPool::setUserData");
    static_cast<QueryPoolCommon*>(this)->userData = value;
}

void CommandBuffer::writeTimestamp(QueryPool* pool, uint32_t index)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::writeTimestamp");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, QueryPoolCommon, pool, "QueryPool",
                    "CommandBuffer::writeTimestamp");
    auto* self = static_cast<CommandBufferCommon*>(this);
    auto* object = static_cast<QueryPoolCommon*>(pool);
    [[maybe_unused]] InstanceCommon* instance = self->device->instance;
    SLAG_VALIDATE(instance, self->state == CommandBufferState::Recording, "CommandBuffer::writeTimestamp",
                  "not recording");
    SLAG_VALIDATE(instance, object && index < object->count, "CommandBuffer::writeTimestamp",
                  "the pool is null or index %u is out of range", index);
    SLAG_VALIDATE(instance, !object || object->type == QueryType::Timestamp, "CommandBuffer::writeTimestamp",
                  "the pool doesn't hold timestamps");
    SLAG_VALIDATE(instance,
                  !object ||
                      (object->queueType == CommandQueueType::Copy) == (self->queueType == CommandQueueType::Copy),
                  "CommandBuffer::writeTimestamp", "copy-queue pools are only used on copy queues, and the reverse");
    SLAG_DISPATCH(self, cmdWriteTimestamp)(self, object, index);
}

void CommandBuffer::resolveQueries(QueryPool* pool, uint32_t first, uint32_t count, Buffer* buffer, uint64_t offset)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::resolveQueries");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, QueryPoolCommon, pool, "QueryPool",
                    "CommandBuffer::resolveQueries");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, BufferCommon, buffer, "Buffer",
                    "CommandBuffer::resolveQueries");
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

void CommandBuffer::beginQuery(QueryPool* pool, uint32_t index)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::beginQuery");
    auto* self = static_cast<CommandBufferCommon*>(this);
    auto* object = static_cast<QueryPoolCommon*>(pool);
    [[maybe_unused]] InstanceCommon* instance = self->device->instance;
    [[maybe_unused]] const char* function = "CommandBuffer::beginQuery";
    SLAG_VALIDATE_LIVE(instance, object, "QueryPool", function);
    SLAG_VALIDATE(instance, self->state == CommandBufferState::Recording && self->inRenderPass, function,
                  "not inside a render pass");
    SLAG_VALIDATE(instance, object && object->type == QueryType::Occlusion && index < object->count, function,
                  "the pool is null, not an occlusion pool, or index %u is out of range", index);
    SLAG_VALIDATE(instance, !self->activeQueryPool, function, "another occlusion query is still open");
    self->activeQueryPool = object;
    self->activeQueryIndex = index;
    SLAG_DISPATCH(self, cmdBeginQuery)(self, object, index);
}

void CommandBuffer::endQuery(QueryPool* pool, uint32_t index)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::endQuery");
    auto* self = static_cast<CommandBufferCommon*>(this);
    auto* object = static_cast<QueryPoolCommon*>(pool);
    [[maybe_unused]] InstanceCommon* instance = self->device->instance;
    SLAG_VALIDATE_LIVE(instance, object, "QueryPool", "CommandBuffer::endQuery");
    SLAG_VALIDATE(instance, object && self->activeQueryPool == object && self->activeQueryIndex == index,
                  "CommandBuffer::endQuery", "query %u of this pool isn't the open one", index);
    self->activeQueryPool = nullptr;
    SLAG_DISPATCH(self, cmdEndQuery)(self, object, index);
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
