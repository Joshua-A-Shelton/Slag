#include "common/Logging.h"
#include "common/Objects.h"

#include <slag/GraphicsDevice.h>
#include <slag/MemoryBlock.h>

namespace slag {

Result<MemoryBlock*> GraphicsDevice::createMemoryBlock(const MemoryBlockDesc& desc)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::createMemoryBlock",
                  "device \"%s\" is not initialized", self->info.name);
    SLAG_VALIDATE(self->instance, desc.size > 0, "GraphicsDevice::createMemoryBlock", "size is 0 (memory block \"%s\")",
                  desc.debugName ? desc.debugName : "");
    SLAG_VALIDATE(self->instance, desc.kind != MemoryKind::GpuUpload || self->capabilities.gpuUploadMemory,
                  "GraphicsDevice::createMemoryBlock", "GpuUpload memory is not supported on \"%s\"", self->info.name);
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    Result<MemoryBlockCommon*> block = SLAG_DISPATCH(self, createMemoryBlock)(self, desc);
    if (!block)
    {
        return block.status();
    }
    block.value()->userData = desc.userData;
    return static_cast<MemoryBlock*>(block.value());
}

void GraphicsDevice::destroy(MemoryBlock* memoryBlock)
{
    if (!memoryBlock)
    {
        return;
    }
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    auto* object = static_cast<MemoryBlockCommon*>(memoryBlock);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the memory block belongs to another device");
    SLAG_VALIDATE(self->instance, object->liveResources == 0, "GraphicsDevice::destroy",
                  "the memory block still holds %u placed resources", object->liveResources.load());
    SLAG_DISPATCH(self, destroyMemoryBlock)(self, object);
}

uint64_t MemoryBlock::size() const
{
    return static_cast<const MemoryBlockCommon*>(this)->size;
}

MemoryKind MemoryBlock::kind() const
{
    return static_cast<const MemoryBlockCommon*>(this)->kind;
}

uint64_t MemoryBlock::userData() const
{
    return static_cast<const MemoryBlockCommon*>(this)->userData;
}

void MemoryBlock::setUserData(uint64_t value)
{
    static_cast<MemoryBlockCommon*>(this)->userData = value;
}

} // namespace slag
