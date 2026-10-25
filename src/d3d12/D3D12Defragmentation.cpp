#include "d3d12/D3D12Backend.h"

#include <utility>

namespace slag::d3d12 {

void swapBufferNative(BufferCommon* a, BufferCommon* b)
{
    auto* first = static_cast<BufferImpl*>(a);
    auto* second = static_cast<BufferImpl*>(b);
    std::swap(first->resource, second->resource);
    std::swap(first->mapped, second->mapped);
    std::swap(first->gpuAddress, second->gpuAddress);
}

void swapTextureNative(TextureCommon* a, TextureCommon* b)
{
    std::swap(static_cast<TextureImpl*>(a)->resource, static_cast<TextureImpl*>(b)->resource);
}

void swapAttachmentViewNative(AttachmentViewCommon* a, AttachmentViewCommon* b)
{
    std::swap(static_cast<AttachmentViewImpl*>(a)->handle, static_cast<AttachmentViewImpl*>(b)->handle);
}

// Every earlier use of a moved resource is in an earlier submit, finished
// and flushed (ADR 0049), so the barriers start from SYNC_NONE. On graphics
// and compute lists the old texture goes from its layout to COPY_SOURCE and
// the new one (created in COMMON, D3D12Texture.cpp) to COPY_DEST, then to the
// old one's layout; copy lists stay in COMMON throughout. Textures passed as
// Undefined aren't copied (ADR 0062).
void cmdRecordMoves(CommandBufferCommon* object, const MoveEntry* moves, size_t count)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    bool copyList = commandBuffer->type == D3D12_COMMAND_LIST_TYPE_COPY;
    Vector<D3D12_TEXTURE_BARRIER>& barriers = commandBuffer->textureBarriers;
    barriers.clear();

    D3D12_TEXTURE_BARRIER barrier = {};
    barrier.Subresources.IndexOrFirstMipLevel = 0xFFFFFFFF; // every subresource
    barrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
    barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
    barrier.SyncAfter = D3D12_BARRIER_SYNC_COPY;
    for (size_t i = 0; i < count && !copyList; ++i)
    {
        const MoveEntry& move = moves[i];
        if (!move.texture || move.layout == TextureLayout::Undefined)
        {
            continue;
        }
        if (move.layout != TextureLayout::CopySource)
        {
            barrier.AccessAfter = D3D12_BARRIER_ACCESS_COPY_SOURCE;
            barrier.LayoutBefore = barrierLayout(move.layout);
            barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_COPY_SOURCE;
            barrier.pResource = static_cast<TextureImpl*>(move.texture)->resource;
            (void)barriers.pushBack(barrier); // keeps its capacity across reset()
        }
        barrier.AccessAfter = D3D12_BARRIER_ACCESS_COPY_DEST;
        barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
        barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_COPY_DEST;
        barrier.pResource = static_cast<TextureImpl*>(move.newTexture)->resource;
        (void)barriers.pushBack(barrier);
    }
    if (!barriers.empty())
    {
        D3D12_BARRIER_GROUP group = {D3D12_BARRIER_TYPE_TEXTURE, static_cast<UINT32>(barriers.size())};
        group.pTextureBarriers = barriers.data();
        commandBuffer->list->Barrier(1, &group);
    }

    for (size_t i = 0; i < count; ++i)
    {
        const MoveEntry& move = moves[i];
        if (move.buffer)
        {
            commandBuffer->list->CopyBufferRegion(static_cast<BufferImpl*>(move.newBuffer)->resource, 0,
                                                  static_cast<BufferImpl*>(move.buffer)->resource, 0,
                                                  move.buffer->size);
        }
        else if (move.texture && move.layout != TextureLayout::Undefined)
        {
            commandBuffer->list->CopyResource(static_cast<TextureImpl*>(move.newTexture)->resource,
                                              static_cast<TextureImpl*>(move.texture)->resource);
        }
    }

    // The copies finish before anything else touches the new resources; new
    // textures end in the old ones' layouts.
    barriers.clear();
    barrier.SyncBefore = D3D12_BARRIER_SYNC_COPY;
    barrier.AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
    barrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;
    barrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
    barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COPY_DEST;
    for (size_t i = 0; i < count && !copyList; ++i)
    {
        const MoveEntry& move = moves[i];
        if (move.texture && move.layout != TextureLayout::Undefined)
        {
            barrier.LayoutAfter = barrierLayout(move.layout);
            barrier.pResource = static_cast<TextureImpl*>(move.newTexture)->resource;
            (void)barriers.pushBack(barrier);
        }
    }
    D3D12_GLOBAL_BARRIER global = {};
    global.SyncBefore = D3D12_BARRIER_SYNC_COPY;
    global.SyncAfter = D3D12_BARRIER_SYNC_NONE;
    global.AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
    global.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
    D3D12_BARRIER_GROUP groups[2] = {{D3D12_BARRIER_TYPE_GLOBAL, 1}, {D3D12_BARRIER_TYPE_TEXTURE, 0}};
    groups[0].pGlobalBarriers = &global;
    groups[1].NumBarriers = static_cast<UINT32>(barriers.size());
    groups[1].pTextureBarriers = barriers.data();
    commandBuffer->list->Barrier(barriers.empty() ? 1 : 2, groups);
}

} // namespace slag::d3d12
