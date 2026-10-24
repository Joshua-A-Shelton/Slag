#include "common/validation/SlotTracking.h"

#ifdef SLAG_VALIDATION

namespace slag {

namespace {

std::atomic<uint32_t>* referencesOf(const GraphicsDeviceCommon::SlotRecord& record)
{
    const void* object = record.object.load(std::memory_order_relaxed);
    if (!object)
    {
        return nullptr;
    }
    // Slots only ever hold live objects: destroying one releases its slots first.
    if (record.description.content == SlotContent::Texture)
    {
        return &const_cast<TextureCommon*>(static_cast<const TextureCommon*>(object))->slotReferences;
    }
    return &const_cast<BufferCommon*>(static_cast<const BufferCommon*>(object))->slotReferences;
}

// Swaps the slot's object, keeping the reference counts right.
void setSlot(GraphicsDeviceCommon* device, uint32_t index, const void* object, std::atomic<uint32_t>* references,
             uint32_t moveCount, const SlotDescription& description)
{
    if (!device->slotRecords || index >= device->resourceHeapSize)
    {
        return;
    }
    GraphicsDeviceCommon::SlotRecord& record = device->slotRecords[index];
    if (std::atomic<uint32_t>* previous = referencesOf(record))
    {
        --*previous;
    }
    if (references)
    {
        ++*references;
    }
    record.description = description;
    record.object.store(object, std::memory_order_relaxed);
    record.moveCount.store(moveCount, std::memory_order_relaxed);
}

void release(GraphicsDeviceCommon* device, const void* object, std::atomic<uint32_t>& references, const char* typeName,
             const char* name)
{
    uint32_t count = references.load();
    if (count == 0 || !device->slotRecords)
    {
        return;
    }
    // Only when the resource is still in slots: a scan of the whole table.
    uint32_t first = UINT32_MAX;
    for (uint32_t slot = 0; slot < device->resourceHeapSize; ++slot)
    {
        GraphicsDeviceCommon::SlotRecord& record = device->slotRecords[slot];
        if (record.object.load(std::memory_order_relaxed) != object)
        {
            continue;
        }
        first = first == UINT32_MAX ? slot : first;
        SLAG_DISPATCH(device, poisonResourceSlot)(device, slot, record.description);
        record.object.store(nullptr, std::memory_order_relaxed);
    }
    references = 0;
    // Info, not Warning: apps usually overwrite slots later rather than clear them (ADR 0031).
    logMessage(messageSink(device->instance), MessageSeverity::Info, MessageSource::Slag,
               "slag: GraphicsDevice::destroy: %s \"%s\" was still in %u resource slots (first: %u); they now hold "
               "nothing (poison descriptors on D3D12)",
               typeName, name, count, first);
}

} // namespace

void trackBufferSlot(GraphicsDeviceCommon* device, uint32_t index, const BufferDescriptorInfo& info)
{
    SlotDescription description;
    description.content = SlotContent::Buffer;
    description.bufferKind = info.kind;
    description.format = info.format;
    description.stride = info.stride;
    // Constant buffers are written by address, so they hold no object.
    setSlot(device, index, info.buffer, info.buffer ? &info.buffer->slotReferences : nullptr,
            info.buffer ? info.buffer->moveCount : 0, description);
}

void trackTextureSlot(GraphicsDeviceCommon* device, uint32_t index, const TextureDescriptorInfo& info)
{
    SlotDescription description;
    description.content = SlotContent::Texture;
    description.writable = info.writable;
    description.viewType = info.viewType;
    description.format = info.format;
    setSlot(device, index, info.texture, info.texture ? &info.texture->slotReferences : nullptr,
            info.texture ? info.texture->moveCount : 0, description);
}

void clearSlot(GraphicsDeviceCommon* device, uint32_t index)
{
    if (!device->slotRecords || index >= device->resourceHeapSize)
    {
        return;
    }
    SlotDescription description = device->slotRecords[index].description;
    if (description.content != SlotContent::Empty)
    {
        SLAG_DISPATCH(device, poisonResourceSlot)(device, index, description);
    }
    // Keeps the kind, so clearing again poisons the same way.
    setSlot(device, index, nullptr, nullptr, 0, description);
}

void releaseSlots(GraphicsDeviceCommon* device, BufferCommon* buffer)
{
    release(device, buffer, buffer->slotReferences, "buffer", debugNameOf(buffer));
}

void releaseSlots(GraphicsDeviceCommon* device, TextureCommon* texture)
{
    release(device, texture, texture->slotReferences, "texture", debugNameOf(texture));
}

} // namespace slag

#endif
