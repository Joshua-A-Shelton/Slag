#include "common/Logging.h"
#include "common/Objects.h"
#include "common/SubAllocator.h"
#include "common/validation/SlotTracking.h"

#include <slag/Buffer.h>
#include <slag/GraphicsDevice.h>

namespace slag {

Result<Buffer*> GraphicsDevice::createBuffer(const BufferDesc& desc)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::createBuffer", "device \"%s\" is not initialized",
                  self->info.name);
    SLAG_VALIDATE(self->instance, desc.size > 0, "GraphicsDevice::createBuffer", "size is 0 (buffer \"%s\")",
                  desc.debugName ? desc.debugName : "");
    SLAG_VALIDATE(self->instance,
                  !desc.placement.movable || desc.placement.kind == MemoryKind::GpuOnly ||
                      desc.placement.kind == MemoryKind::GpuUpload,
                  "GraphicsDevice::createBuffer", "only GpuOnly and GpuUpload buffers can be movable (ADR 0062)");
    SLAG_VALIDATE(self->instance, desc.size <= self->capabilities.maxBufferSize, "GraphicsDevice::createBuffer",
                  "size %llu exceeds the device's maxBufferSize", static_cast<unsigned long long>(desc.size));
#ifdef SLAG_VALIDATION
    SLAG_CHECK_LIVE(self->instance, MemoryBlockCommon, desc.placement.memoryBlock, "MemoryBlock",
                    "GraphicsDevice::createBuffer");
    if (desc.placement.memoryBlock)
    {
        auto* block = static_cast<const MemoryBlockCommon*>(desc.placement.memoryBlock);
        PlacementInfo placement = SLAG_DISPATCH(self, bufferPlacementInfo)(self, desc);
        SLAG_VALIDATE(self->instance, block->device == self, "GraphicsDevice::createBuffer",
                      "the memory block belongs to another device");
        SLAG_VALIDATE(self->instance, placement.alignment && desc.placement.offset % placement.alignment == 0,
                      "GraphicsDevice::createBuffer", "offset %llu is not a multiple of the alignment (%llu)",
                      static_cast<unsigned long long>(desc.placement.offset),
                      static_cast<unsigned long long>(placement.alignment));
        SLAG_VALIDATE(
            self->instance,
            desc.placement.offset <= block->size && placement.size <= block->size - desc.placement.offset,
            "GraphicsDevice::createBuffer", "the buffer (%llu bytes at offset %llu) ends past the memory block",
            static_cast<unsigned long long>(placement.size), static_cast<unsigned long long>(desc.placement.offset));
    }
#endif
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    if (desc.placement.kind == MemoryKind::GpuUpload && !desc.placement.memoryBlock &&
        !self->capabilities.gpuUploadMemory)
    {
        logMessage(messageSink(self->instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createBuffer: GpuUpload memory is not supported on \"%s\"", self->info.name);
        return Status::Unsupported;
    }

    // Automatic placement: sub-allocated, or dedicated when large (ADR 0061).
    PoolAllocation allocation;
    BufferDesc placed = desc;
    if (!desc.placement.memoryBlock)
    {
        PlacementInfo placement = SLAG_DISPATCH(self, bufferPlacementInfo)(self, desc);
        if (self->subAllocator->fits(desc.placement.kind, placement))
        {
            MemoryBlockCommon* block = nullptr;
            uint64_t offset = 0;
            bool movable = desc.placement.movable &&
                           (desc.placement.kind == MemoryKind::GpuOnly || desc.placement.kind == MemoryKind::GpuUpload);
            Status status =
                self->subAllocator->allocate(desc.placement.kind, placement, movable, allocation, block, offset);
            if (status != Status::Ok)
            {
                return status;
            }
            placed.placement = Placement::placed(block, offset);
        }
    }
    Result<BufferCommon*> buffer = SLAG_DISPATCH(self, createBuffer)(self, placed);
    if (!buffer)
    {
        self->subAllocator->free(allocation);
        return buffer.status();
    }
    if (allocation.block)
    {
        buffer.value()->allocation = allocation;
        self->subAllocator->setOwner(allocation, buffer.value(), false);
    }
    buffer.value()->userData = desc.userData;
    setDebugName(self->instance, buffer.value(), desc.debugName);
    if (desc.placement.memoryBlock)
    {
        buffer.value()->memoryBlock = static_cast<MemoryBlockCommon*>(desc.placement.memoryBlock);
        ++buffer.value()->memoryBlock->liveResources;
    }
    return static_cast<Buffer*>(buffer.value());
}

void GraphicsDevice::destroy(Buffer* buffer)
{
    SLAG_CHECK_LIVE(static_cast<const GraphicsDeviceCommon*>(this)->instance, BufferCommon, buffer, "Buffer",
                    "GraphicsDevice::destroy");
    if (!buffer)
    {
        return;
    }
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    auto* object = static_cast<BufferCommon*>(buffer);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the buffer belongs to another device");
    SLAG_VALIDATE(self->instance, !object->movePass, "GraphicsDevice::destroy",
                  "the buffer is being moved by the active defragmentation pass; its move is dropped");
    if (object->movePass)
    {
        dropMove(object->movePass, object->moveIndex);
    }
#ifdef SLAG_VALIDATION
    releaseSlots(self, object);
#endif
    MemoryBlockCommon* block = object->memoryBlock;
    PoolAllocation allocation = object->allocation;
    SLAG_DISPATCH(self, destroyBuffer)(self, object);
    if (block)
    {
        --block->liveResources;
    }
    self->subAllocator->free(allocation);
}

PlacementInfo GraphicsDevice::placementInfo(const BufferDesc& desc) const
{
    auto* self = static_cast<const GraphicsDeviceCommon*>(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::placementInfo",
                  "device \"%s\" is not initialized", self->info.name);
    return SLAG_DISPATCH(self, bufferPlacementInfo)(self, desc);
}

uint64_t Buffer::gpuAddress() const
{
    SLAG_CHECK_LIVE_THIS(BufferCommon, "Buffer", "Buffer::gpuAddress");
    return static_cast<const BufferCommon*>(this)->gpuAddress;
}

void* Buffer::mappedData() const
{
    SLAG_CHECK_LIVE_THIS(BufferCommon, "Buffer", "Buffer::mappedData");
    return static_cast<const BufferCommon*>(this)->mapped;
}

uint64_t Buffer::userData() const
{
    SLAG_CHECK_LIVE_THIS(BufferCommon, "Buffer", "Buffer::userData");
    return static_cast<const BufferCommon*>(this)->userData;
}

void Buffer::setUserData(uint64_t value)
{
    SLAG_CHECK_LIVE_THIS(BufferCommon, "Buffer", "Buffer::setUserData");
    static_cast<BufferCommon*>(this)->userData = value;
}

} // namespace slag
