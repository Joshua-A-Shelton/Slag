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
    if (!self->initialized)
    {
        return Status::Unsupported; // reported above; nothing to create it on
    }
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
    setDebugName(self->instance, block.value(), desc.debugName);
    return static_cast<MemoryBlock*>(block.value());
}

void GraphicsDevice::destroy(MemoryBlock* memoryBlock)
{
    SLAG_CHECK_LIVE(static_cast<const GraphicsDeviceCommon*>(this)->instance, MemoryBlockCommon, memoryBlock,
                    "MemoryBlock", "GraphicsDevice::destroy");
#ifdef SLAG_VALIDATION
    if (memoryBlock && static_cast<const MemoryBlockCommon*>(memoryBlock)->destroyed)
    {
        return; // reported above: destroying it again would free it twice
    }
#endif
    if (!memoryBlock)
    {
        return;
    }
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    auto* object = static_cast<MemoryBlockCommon*>(memoryBlock);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the memory block belongs to another device");
    if (object->device != self)
    {
        return; // the other device's state isn't this one's to change
    }
    SLAG_VALIDATE(self->instance, object->liveResources == 0, "GraphicsDevice::destroy",
                  "the memory block still holds %u placed resources", object->liveResources.load());
    SLAG_DISPATCH(self, destroyMemoryBlock)(self, object);
}

uint64_t MemoryBlock::size() const
{
    SLAG_CHECK_LIVE_THIS(MemoryBlockCommon, "MemoryBlock", "MemoryBlock::size");
    return static_cast<const MemoryBlockCommon*>(this)->size;
}

MemoryKind MemoryBlock::kind() const
{
    SLAG_CHECK_LIVE_THIS(MemoryBlockCommon, "MemoryBlock", "MemoryBlock::kind");
    return static_cast<const MemoryBlockCommon*>(this)->kind;
}

uint64_t MemoryBlock::userData() const
{
    SLAG_CHECK_LIVE_THIS(MemoryBlockCommon, "MemoryBlock", "MemoryBlock::userData");
    return static_cast<const MemoryBlockCommon*>(this)->userData;
}

void MemoryBlock::setUserData(uint64_t value)
{
    SLAG_CHECK_LIVE_THIS(MemoryBlockCommon, "MemoryBlock", "MemoryBlock::setUserData");
    static_cast<MemoryBlockCommon*>(this)->userData = value;
}

} // namespace slag
