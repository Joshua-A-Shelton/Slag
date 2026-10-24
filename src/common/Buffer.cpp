#include "common/Logging.h"
#include "common/Objects.h"

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
    SLAG_VALIDATE(self->instance, desc.size <= self->capabilities.maxBufferSize, "GraphicsDevice::createBuffer",
                  "size %llu exceeds the device's maxBufferSize", static_cast<unsigned long long>(desc.size));
#ifdef SLAG_VALIDATION
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
    Result<BufferCommon*> buffer = SLAG_DISPATCH(self, createBuffer)(self, desc);
    if (!buffer)
    {
        return buffer.status();
    }
    buffer.value()->userData = desc.userData;
    if (desc.placement.memoryBlock)
    {
        buffer.value()->memoryBlock = static_cast<MemoryBlockCommon*>(desc.placement.memoryBlock);
        ++buffer.value()->memoryBlock->liveResources;
    }
    return static_cast<Buffer*>(buffer.value());
}

void GraphicsDevice::destroy(Buffer* buffer)
{
    if (!buffer)
    {
        return;
    }
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    auto* object = static_cast<BufferCommon*>(buffer);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the buffer belongs to another device");
    MemoryBlockCommon* block = object->memoryBlock;
    SLAG_DISPATCH(self, destroyBuffer)(self, object);
    if (block)
    {
        --block->liveResources;
    }
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
    return static_cast<const BufferCommon*>(this)->gpuAddress;
}

void* Buffer::mappedData() const
{
    return static_cast<const BufferCommon*>(this)->mapped;
}

uint64_t Buffer::userData() const
{
    return static_cast<const BufferCommon*>(this)->userData;
}

void Buffer::setUserData(uint64_t value)
{
    static_cast<BufferCommon*>(this)->userData = value;
}

} // namespace slag
