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

// New textures start in COMMON (D3D12Texture.cpp). On graphics and compute
// lists they go to COPY_DEST for the copy and end in COPY_SOURCE, the layout
// the old ones are in; copy lists use COMMON throughout (ADR 0062).
void cmdRecordMoves(CommandBufferCommon* object, const MoveEntry* moves, size_t count)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    bool copyList = commandBuffer->type == D3D12_COMMAND_LIST_TYPE_COPY;
    Vector<D3D12_TEXTURE_BARRIER>& barriers = commandBuffer->textureBarriers;
    barriers.clear();

    D3D12_TEXTURE_BARRIER barrier = {};
    barrier.Subresources.IndexOrFirstMipLevel = 0xFFFFFFFF; // every subresource
    if (!copyList)
    {
        barrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
        barrier.SyncAfter = D3D12_BARRIER_SYNC_COPY;
        barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
        barrier.AccessAfter = D3D12_BARRIER_ACCESS_COPY_DEST;
        barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
        barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_COPY_DEST;
        for (size_t i = 0; i < count; ++i)
        {
            if (moves[i].texture)
            {
                barrier.pResource = static_cast<TextureImpl*>(moves[i].newTexture)->resource;
                (void)barriers.pushBack(barrier); // keeps its capacity across reset()
            }
        }
        if (!barriers.empty())
        {
            D3D12_BARRIER_GROUP group = {D3D12_BARRIER_TYPE_TEXTURE, static_cast<UINT32>(barriers.size())};
            group.pTextureBarriers = barriers.data();
            commandBuffer->list->Barrier(1, &group);
        }
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
        else if (move.texture)
        {
            commandBuffer->list->CopyResource(static_cast<TextureImpl*>(move.newTexture)->resource,
                                              static_cast<TextureImpl*>(move.texture)->resource);
        }
    }

    // The copies finish before anything else touches the new resources.
    D3D12_GLOBAL_BARRIER global = {};
    global.SyncBefore = D3D12_BARRIER_SYNC_COPY;
    global.SyncAfter = D3D12_BARRIER_SYNC_NONE;
    global.AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
    global.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
    D3D12_BARRIER_GROUP groups[2] = {{D3D12_BARRIER_TYPE_GLOBAL, 1}, {D3D12_BARRIER_TYPE_TEXTURE, 0}};
    groups[0].pGlobalBarriers = &global;
    for (D3D12_TEXTURE_BARRIER& texture : barriers)
    {
        texture.SyncBefore = D3D12_BARRIER_SYNC_COPY;
        texture.SyncAfter = D3D12_BARRIER_SYNC_NONE;
        texture.AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
        texture.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
        texture.LayoutBefore = D3D12_BARRIER_LAYOUT_COPY_DEST;
        texture.LayoutAfter = D3D12_BARRIER_LAYOUT_COPY_SOURCE;
    }
    groups[1].NumBarriers = static_cast<UINT32>(barriers.size());
    groups[1].pTextureBarriers = barriers.data();
    commandBuffer->list->Barrier(barriers.empty() ? 1 : 2, groups);
}

} // namespace slag::d3d12
