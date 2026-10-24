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
    if (desc.placement.memoryBlock)
    {
        // Placed resources arrive with memory blocks in milestone M2.
        logMessage(messageSink(self->instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createBuffer: placed buffers are not implemented yet");
        return Status::Unsupported;
    }
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
    SLAG_DISPATCH(self, destroyBuffer)(self, object);
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
