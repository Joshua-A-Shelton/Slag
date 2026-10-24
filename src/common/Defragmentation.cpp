#include "common/Logging.h"
#include "common/Objects.h"
#include "common/SubAllocator.h"

#include <slag/CommandBuffer.h>
#include <slag/Defragmentation.h>
#include <slag/GraphicsDevice.h>

namespace slag {

namespace {

// The description of a moved texture, for its new native object.
TextureCreateInfo recreateInfo(const TextureCommon& texture, MemoryBlockCommon* block, uint64_t offset)
{
    TextureCreateInfo info;
    info.type = texture.type;
    info.format = texture.format;
    info.width = texture.width;
    info.height = texture.height;
    info.depth = texture.depth;
    info.layerCount = texture.layerCount;
    info.mipCount = texture.mipCount;
    info.sampleCount = texture.sampleCount;
    info.usage = texture.usage;
    info.castableFormats = {texture.castableFormats, texture.castableCount};
    info.clearValue = texture.hasClearValue ? &texture.clearValue : nullptr;
    info.placement = Placement::placed(block, offset);
    info.moveTarget = true;
    info.debugName = texture.debugName; // the native object keeps its name (ADR 0017)
    return info;
}

void destroyPass(GraphicsDeviceCommon* device, DefragmentationPassCommon* pass)
{
    device->instance->allocator.destroy(pass);
}

// The new native objects of every attachment view of a moved texture.
Status rebuildViews(GraphicsDeviceCommon* device, TextureCommon* texture)
{
    Status status = Status::Ok;
    std::lock_guard<std::mutex> lock(device->viewListLock);
    for (AttachmentViewCommon* view = texture->views; view; view = view->nextView)
    {
        AttachmentViewDesc desc;
        desc.texture = texture;
        desc.format = view->format;
        desc.mip = view->mip;
        desc.baseLayer = view->baseLayer;
        desc.layerCount = view->layerCount;
        desc.debugName = view->debugName;
        Result<AttachmentViewCommon*> rebuilt = SLAG_DISPATCH(device, createAttachmentView)(device, desc);
        if (!rebuilt)
        {
            status = rebuilt.status();
            continue;
        }
        SLAG_DISPATCH(device, swapAttachmentViewNative)(view, rebuilt.value());
        SLAG_DISPATCH(device, destroyAttachmentView)(device, rebuilt.value());
    }
    return status;
}

#ifdef SLAG_VALIDATION
// Slots still holding a resource this pass moved, for the next submit to
// report (ADR 0062). `moved` is sorted.
void findStaleSlots(GraphicsDeviceCommon* device, const Vector<const void*>& moved)
{
    if (!device->slotRecords || moved.empty())
    {
        return;
    }
    std::lock_guard<std::mutex> lock(device->staleSlotLock);
    for (uint32_t slot = 0; slot < device->resourceHeapSize; ++slot)
    {
        const void* object = device->slotRecords[slot].object.load(std::memory_order_relaxed);
        if (!object)
        {
            continue;
        }
        size_t low = 0;
        size_t high = moved.size();
        while (low < high)
        {
            size_t middle = (low + high) / 2;
            if (moved[middle] < object)
            {
                low = middle + 1;
            }
            else
            {
                high = middle;
            }
        }
        if (low < moved.size() && moved[low] == object)
        {
            (void)device->staleSlots.pushBack(
                {slot, object, device->slotRecords[slot].moveCount.load(std::memory_order_relaxed)});
        }
    }
    device->hasStaleSlots = !device->staleSlots.empty();
}
#endif

} // namespace

void dropMove(DefragmentationPassCommon* pass, uint32_t index)
{
    GraphicsDeviceCommon* device = pass->device;
    MoveEntry& entry = pass->entries[index];
    if (entry.newBuffer)
    {
        SLAG_DISPATCH(device, destroyBuffer)(device, entry.newBuffer);
        entry.buffer->movePass = nullptr;
    }
    if (entry.newTexture)
    {
        SLAG_DISPATCH(device, destroyTexture)(device, entry.newTexture);
        entry.texture->movePass = nullptr;
    }
    device->subAllocator->free(entry.to);
    entry = {};
    pass->moves[index] = {};
}

Result<DefragmentationPass*> GraphicsDevice::beginDefragmentation(const DefragmentationDesc& desc)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    [[maybe_unused]] const char* function = "GraphicsDevice::beginDefragmentation";
    SLAG_VALIDATE(self->instance, self->initialized, function, "device \"%s\" is not initialized", self->info.name);
    SLAG_VALIDATE(self->instance, !self->activePass, function, "a defragmentation pass is already active");
    SLAG_VALIDATE(self->instance, desc.kind == MemoryKind::GpuOnly || desc.kind == MemoryKind::GpuUpload, function,
                  "only GpuOnly and GpuUpload memory is defragmented");
    if (!self->initialized)
    {
        return Status::Unsupported; // reported above; nothing to create it on
    }
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    if (self->activePass || (desc.kind != MemoryKind::GpuOnly && desc.kind != MemoryKind::GpuUpload))
    {
        return Status::Unsupported;
    }
    const HostAllocator* allocator = &self->instance->allocator;
    auto* pass = allocator->create<DefragmentationPassCommon>();
    if (!pass)
    {
        return Status::OutOfHostMemory;
    }
    pass->device = self;
    pass->kind = desc.kind;
    pass->moves = Vector<DefragmentationMove>(allocator);
    pass->entries = Vector<MoveEntry>(allocator);

    Vector<PlannedMove> planned(allocator);
    Status status = self->subAllocator->planDefragmentation(desc.kind, desc.maxBytes, desc.maxMoves, planned);
    if (status == Status::Ok && (!pass->moves.reserve(planned.size()) || !pass->entries.reserve(planned.size())))
    {
        status = Status::OutOfHostMemory;
    }
    if (status != Status::Ok)
    {
        for (PlannedMove& move : planned)
        {
            self->subAllocator->free(move.to);
        }
        self->subAllocator->endDefragmentation(desc.kind);
        destroyPass(self, pass);
        return status;
    }

    // A new native object at each new range. One that can't be created is
    // left out of the pass, with its range given back.
    for (PlannedMove& move : planned)
    {
        MoveEntry entry;
        entry.from = move.from;
        entry.to = move.to;
        DefragmentationMove publicMove;
        if (move.ownerIsTexture)
        {
            auto* texture = static_cast<TextureCommon*>(move.owner);
            Result<TextureCommon*> created =
                SLAG_DISPATCH(self, createTexture)(self, recreateInfo(*texture, move.block, move.offset));
            if (!created)
            {
                self->subAllocator->free(move.to);
                continue;
            }
            entry.texture = texture;
            entry.newTexture = created.value();
            texture->movePass = pass;
            texture->moveIndex = static_cast<uint32_t>(pass->entries.size());
            publicMove.texture = texture;
        }
        else
        {
            auto* buffer = static_cast<BufferCommon*>(move.owner);
            BufferDesc bufferDesc;
            bufferDesc.size = buffer->size;
            bufferDesc.placement = Placement::placed(move.block, move.offset);
            bufferDesc.debugName = buffer->debugName;
            Result<BufferCommon*> created = SLAG_DISPATCH(self, createBuffer)(self, bufferDesc);
            if (!created)
            {
                self->subAllocator->free(move.to);
                continue;
            }
            entry.buffer = buffer;
            entry.newBuffer = created.value();
            buffer->movePass = pass;
            buffer->moveIndex = static_cast<uint32_t>(pass->entries.size());
            publicMove.buffer = buffer;
        }
        (void)pass->entries.pushBack(entry); // reserved above
        (void)pass->moves.pushBack(publicMove);
    }
    self->activePass = pass;
    return static_cast<DefragmentationPass*>(pass);
}

std::span<const DefragmentationMove> DefragmentationPass::moves() const
{
    auto* self = static_cast<const DefragmentationPassCommon*>(this);
    return {self->moves.data(), self->moves.size()};
}

void DefragmentationPass::recordCopies(CommandBuffer* commandBuffer)
{
    SLAG_CHECK_LIVE(static_cast<const DefragmentationPassCommon*>(this)->device->instance, CommandBufferCommon,
                    commandBuffer, "CommandBuffer", "DefragmentationPass::recordCopies");
    auto* self = static_cast<DefragmentationPassCommon*>(this);
    auto* object = static_cast<CommandBufferCommon*>(commandBuffer);
    [[maybe_unused]] const char* function = "DefragmentationPass::recordCopies";
    SLAG_VALIDATE(self->device->instance, object && object->state == CommandBufferState::Recording, function,
                  "the command buffer is not recording");
    SLAG_VALIDATE(self->device->instance, !object || !object->inRenderPass, function, "inside a render pass");
    SLAG_VALIDATE(self->device->instance, !self->recorded, function, "the copies are already recorded");
    if (!object)
    {
        return;
    }
    SLAG_DISPATCH(object, cmdRecordMoves)(object, self->entries.data(), self->entries.size());
    self->recorded = true;
}

Result<void> GraphicsDevice::endDefragmentation(DefragmentationPass* pass, [[maybe_unused]] SemaphoreValue completed)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    auto* object = static_cast<DefragmentationPassCommon*>(pass);
    [[maybe_unused]] const char* function = "GraphicsDevice::endDefragmentation";
    SLAG_VALIDATE(self->instance, object && object == self->activePass, function,
                  "the pass is null or not this device's active pass");
    if (!object || object != self->activePass)
    {
        return Status::Unsupported;
    }
#ifdef SLAG_VALIDATION
    bool anyMoves = false;
    for (const MoveEntry& entry : object->entries)
    {
        anyMoves = anyMoves || entry.buffer || entry.texture;
    }
    SLAG_VALIDATE(self->instance, !anyMoves || object->recorded, function,
                  "the pass has moves, but recordCopies wasn't called");
    SLAG_VALIDATE(self->instance,
                  !anyMoves || self->lost || (completed.semaphore && completed.semaphore->value() >= completed.value),
                  function, "the semaphore value of the copies' completion hasn't been reached");
    Vector<const void*> moved(&self->instance->allocator);
#endif

    // Swap the new native objects in; the temporaries then hold the old ones.
    Status status = Status::Ok;
    for (MoveEntry& entry : object->entries)
    {
        if (entry.buffer)
        {
            SLAG_DISPATCH(self, swapBufferNative)(entry.buffer, entry.newBuffer);
            SLAG_DISPATCH(self, destroyBuffer)(self, entry.newBuffer);
            self->subAllocator->free(entry.buffer->allocation);
            entry.buffer->allocation = entry.to;
            self->subAllocator->setOwner(entry.to, entry.buffer, false);
            entry.buffer->movePass = nullptr;
            ++entry.buffer->moveCount;
#ifdef SLAG_VALIDATION
            (void)moved.pushBack(entry.buffer);
#endif
        }
        else if (entry.texture)
        {
            SLAG_DISPATCH(self, swapTextureNative)(entry.texture, entry.newTexture);
            SLAG_DISPATCH(self, destroyTexture)(self, entry.newTexture);
            self->subAllocator->free(entry.texture->allocation);
            entry.texture->allocation = entry.to;
            self->subAllocator->setOwner(entry.to, entry.texture, true);
            entry.texture->movePass = nullptr;
            ++entry.texture->moveCount;
            Status viewStatus = rebuildViews(self, entry.texture);
            status = status == Status::Ok ? viewStatus : status;
#ifdef SLAG_VALIDATION
            (void)moved.pushBack(entry.texture);
#endif
        }
    }
    self->subAllocator->endDefragmentation(object->kind);
    self->activePass = nullptr;
    destroyPass(self, object);
#ifdef SLAG_VALIDATION
    // Insertion sort: passes hold few moves.
    for (size_t i = 1; i < moved.size(); ++i)
    {
        const void* value = moved[i];
        size_t j = i;
        while (j > 0 && moved[j - 1] > value)
        {
            moved[j] = moved[j - 1];
            --j;
        }
        moved[j] = value;
    }
    findStaleSlots(self, moved);
#endif
    return status;
}

#ifdef SLAG_VALIDATION
// Called by CommandQueue::submit: reports slots a pass left stale, once.
void reportStaleSlots(GraphicsDeviceCommon* device)
{
    std::lock_guard<std::mutex> lock(device->staleSlotLock);
    for (const GraphicsDeviceCommon::StaleSlot& stale : device->staleSlots)
    {
        const GraphicsDeviceCommon::SlotRecord& record = device->slotRecords[stale.slot];
        bool unchanged = record.object.load(std::memory_order_relaxed) == stale.object &&
                         record.moveCount.load(std::memory_order_relaxed) == stale.moveCount;
        SLAG_VALIDATE(device->instance, !unchanged, "CommandQueue::submit",
                      "resource slot %u still holds the descriptor of a resource moved by defragmentation; write it "
                      "again (or clear it)",
                      stale.slot);
    }
    device->staleSlots.clear();
    device->hasStaleSlots = false;
}
#endif

} // namespace slag
