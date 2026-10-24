#include "d3d12/D3D12Backend.h"
#include "d3d12/D3D12Formats.h"

static_assert(sizeof(slag::DrawArguments) == sizeof(D3D12_DRAW_ARGUMENTS));
static_assert(sizeof(slag::DrawIndexedArguments) == sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));
static_assert(sizeof(slag::DispatchArguments) == sizeof(D3D12_DISPATCH_ARGUMENTS));

namespace slag::d3d12 {

namespace {

ID3D12GraphicsCommandList7* listOf(CommandBufferCommon* commandBuffer)
{
    return static_cast<CommandBufferImpl*>(commandBuffer)->list;
}

D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE beginning(LoadOp op)
{
    switch (op)
    {
    case LoadOp::Load:
        return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
    case LoadOp::Clear:
        return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
    case LoadOp::Discard:
        return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
    }
    return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
}

D3D12_RENDER_PASS_ENDING_ACCESS_TYPE ending(StoreOp op)
{
    switch (op)
    {
    case StoreOp::Store:
        return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    case StoreOp::Discard:
        return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
    case StoreOp::Resolve:
        return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
    }
    return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
}

} // namespace

void cmdBeginRendering(CommandBufferCommon* object, const RenderingDesc& desc, uint32_t width, uint32_t height,
                       uint32_t)
{
    ID3D12GraphicsCommandList7* list = listOf(object);
    D3D12_RENDER_PASS_RENDER_TARGET_DESC targets[8] = {};
    // Resolve parameters live until BeginRenderPass returns.
    D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS resolves[8] = {};
    UINT targetCount = static_cast<UINT>(desc.colorAttachments.size() < 8 ? desc.colorAttachments.size() : 8);
    for (UINT i = 0; i < targetCount; ++i)
    {
        const ColorAttachment& attachment = desc.colorAttachments[i];
        D3D12_RENDER_PASS_RENDER_TARGET_DESC& target = targets[i];
        auto* view = static_cast<AttachmentViewImpl*>(attachment.view);
        if (!view)
        {
            // A gap: a null descriptor with no access.
            target.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
            target.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
            continue;
        }
        target.cpuDescriptor = view->handle;
        target.BeginningAccess.Type = beginning(attachment.loadOp);
        target.BeginningAccess.Clear.ClearValue.Format = toDxgiFormat(view->format);
        for (int c = 0; c < 4; ++c)
        {
            target.BeginningAccess.Clear.ClearValue.Color[c] = attachment.clearValue.color[c];
        }
        target.EndingAccess.Type = ending(attachment.storeOp);
        if (attachment.storeOp == StoreOp::Resolve && attachment.resolveView)
        {
            auto* source = static_cast<TextureImpl*>(view->texture);
            auto* resolve = static_cast<AttachmentViewImpl*>(attachment.resolveView);
            auto* destination = static_cast<TextureImpl*>(resolve->texture);
            resolves[i].SrcSubresource = view->mip + view->baseLayer * source->mipCount;
            resolves[i].DstSubresource = resolve->mip + resolve->baseLayer * destination->mipCount;
            resolves[i].SrcRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
            D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_PARAMETERS& parameters = target.EndingAccess.Resolve;
            parameters.pSrcResource = source->resource;
            parameters.pDstResource = destination->resource;
            parameters.SubresourceCount = 1;
            parameters.pSubresourceParameters = &resolves[i];
            parameters.Format = toDxgiFormat(view->format);
            parameters.ResolveMode = D3D12_RESOLVE_MODE_AVERAGE;
            parameters.PreserveResolveSource = FALSE;
        }
    }

    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencil = {};
    const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* depthStencilDesc = nullptr;
    if (desc.depthStencilAttachment && desc.depthStencilAttachment->view)
    {
        const DepthStencilAttachment& attachment = *desc.depthStencilAttachment;
        auto* view = static_cast<AttachmentViewImpl*>(attachment.view);
        bool stencil = hasTraits(view->format, FormatTraits::Stencil);
        // The read-only DSV variant (ADR 0035).
        uint32_t variant = (attachment.readOnlyDepth ? DsvReadOnlyDepth : 0) |
                           (stencil && attachment.readOnlyStencil ? DsvReadOnlyStencil : 0);
        if (!stencil && attachment.readOnlyDepth)
        {
            variant = DsvReadOnlyDepth;
        }
        UINT increment = static_cast<GraphicsDeviceImpl*>(object->device)->dsvPool.increment;
        depthStencil.cpuDescriptor = {view->handle.ptr + SIZE_T(variant) * increment};
        D3D12_CLEAR_VALUE clear = {};
        clear.Format = toDxgiFormat(view->format);
        clear.DepthStencil = {attachment.clearValue.depth, attachment.clearValue.stencil};
        depthStencil.DepthBeginningAccess.Type = beginning(attachment.depthLoadOp);
        depthStencil.DepthBeginningAccess.Clear.ClearValue = clear;
        depthStencil.DepthEndingAccess.Type = ending(attachment.depthStoreOp);
        if (stencil)
        {
            depthStencil.StencilBeginningAccess.Type = beginning(attachment.stencilLoadOp);
            depthStencil.StencilBeginningAccess.Clear.ClearValue = clear;
            depthStencil.StencilEndingAccess.Type = ending(attachment.stencilStoreOp);
        }
        else
        {
            depthStencil.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
            depthStencil.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
        }
        depthStencilDesc = &depthStencil;
    }

    D3D12_RENDER_PASS_FLAGS flags = D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES; // always (ADR 0046)
    // Read-only planes take both the read-only DSV variant and these flags.
    if (depthStencilDesc && desc.depthStencilAttachment->readOnlyDepth)
    {
        flags |= D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_DEPTH;
    }
    if (depthStencilDesc && desc.depthStencilAttachment->readOnlyStencil &&
        hasTraits(static_cast<AttachmentViewImpl*>(desc.depthStencilAttachment->view)->format, FormatTraits::Stencil))
    {
        flags |= D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_STENCIL;
    }
    if (desc.suspending)
    {
        flags |= D3D12_RENDER_PASS_FLAG_SUSPENDING_PASS;
    }
    if (desc.resuming)
    {
        flags |= D3D12_RENDER_PASS_FLAG_RESUMING_PASS;
    }
    list->BeginRenderPass(targetCount, targets, depthStencilDesc, flags);

    // The full render area to start with (CommandBuffer::beginRendering).
    D3D12_VIEWPORT viewport = {0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f};
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    list->RSSetViewports(1, &viewport);
    list->RSSetScissorRects(1, &scissor);
}

void cmdEndRendering(CommandBufferCommon* object)
{
    listOf(object)->EndRenderPass();
}

void cmdBindVertexBuffers(CommandBufferCommon* object, uint32_t firstSlot,
                          std::span<const VertexBufferBinding> bindings)
{
    D3D12_VERTEX_BUFFER_VIEW views[32] = {};
    UINT count = static_cast<UINT>(bindings.size() < 32 ? bindings.size() : 32);
    for (UINT i = 0; i < count; ++i)
    {
        auto* buffer = static_cast<BufferImpl*>(bindings[i].buffer);
        views[i] = {buffer->gpuAddress + bindings[i].offset, static_cast<UINT>(bindings[i].size), bindings[i].stride};
    }
    listOf(object)->IASetVertexBuffers(firstSlot, count, views);
}

void cmdBindIndexBuffer(CommandBufferCommon* object, BufferCommon* buffer, uint64_t offset, uint64_t size,
                        IndexFormat format)
{
    D3D12_INDEX_BUFFER_VIEW view = {buffer->gpuAddress + offset, static_cast<UINT>(size),
                                    format == IndexFormat::Uint16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT};
    listOf(object)->IASetIndexBuffer(&view);
}

void cmdSetViewports(CommandBufferCommon* object, std::span<const Viewport> viewports)
{
    D3D12_VIEWPORT converted[16];
    UINT count = static_cast<UINT>(viewports.size() < 16 ? viewports.size() : 16);
    for (UINT i = 0; i < count; ++i)
    {
        const Viewport& v = viewports[i];
        converted[i] = {v.x, v.y, v.width, v.height, v.minDepth, v.maxDepth};
    }
    listOf(object)->RSSetViewports(count, converted);
}

void cmdSetScissors(CommandBufferCommon* object, std::span<const Rect> scissors)
{
    D3D12_RECT converted[16];
    UINT count = static_cast<UINT>(scissors.size() < 16 ? scissors.size() : 16);
    for (UINT i = 0; i < count; ++i)
    {
        const Rect& r = scissors[i];
        converted[i] = {r.x, r.y, static_cast<LONG>(r.x + static_cast<int32_t>(r.width)),
                        static_cast<LONG>(r.y + static_cast<int32_t>(r.height))};
    }
    listOf(object)->RSSetScissorRects(count, converted);
}

void cmdSetBlendConstant(CommandBufferCommon* object, const float constant[4])
{
    listOf(object)->OMSetBlendFactor(constant);
}

void cmdSetStencilReference(CommandBufferCommon* object, uint8_t reference)
{
    listOf(object)->OMSetStencilRef(reference);
}

void cmdSetDepthBounds(CommandBufferCommon* object, float minDepth, float maxDepth)
{
    listOf(object)->OMSetDepthBounds(minDepth, maxDepth);
}

void cmdDraw(CommandBufferCommon* object, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
             uint32_t firstInstance)
{
    listOf(object)->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void cmdDrawIndexed(CommandBufferCommon* object, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                    int32_t vertexOffset, uint32_t firstInstance)
{
    listOf(object)->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void cmdIndirect(CommandBufferCommon* object, IndirectKind kind, BufferCommon* argumentBuffer, uint64_t argumentOffset,
                 uint32_t maxCount, BufferCommon* countBuffer, uint64_t countOffset)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(object->device);
    ID3D12Resource* count = countBuffer ? static_cast<BufferImpl*>(countBuffer)->resource : nullptr;
    listOf(object)->ExecuteIndirect(device.commandSignatures[static_cast<size_t>(kind)], maxCount,
                                    static_cast<BufferImpl*>(argumentBuffer)->resource, argumentOffset, count,
                                    countOffset);
}

} // namespace slag::d3d12
