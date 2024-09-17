#ifndef SLAG_GPUMEMORYBARRIERS_H
#define SLAG_GPUMEMORYBARRIERS_H

#define MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION) \
DEFINITION(None, 0b0000000000000000, VK_ACCESS_NONE , D3D12_BARRIER_ACCESS_NO_ACCESS) \
DEFINITION(IndirectCommand, 0b0000000000000001, VK_ACCESS_INDIRECT_COMMAND_READ_BIT , D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT) \
DEFINITION(Index, 0b0000000000000010, VK_ACCESS_INDEX_READ_BIT , D3D12_BARRIER_ACCESS_INDEX_BUFFER) \
DEFINITION(VertexAttribute, 0b0000000000000100, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT , D3D12_BARRIER_ACCESS_VERTEX_BUFFER) \
DEFINITION(Uniform, 0b0000000000001000, VK_ACCESS_UNIFORM_READ_BIT , D3D12_BARRIER_ACCESS_CONSTANT_BUFFER) \
DEFINITION(InputAttachment, 0b0000000000010000, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT , D3D12_BARRIER_ACCESS_SHADER_RESOURCE) \
DEFINITION(ShaderRead, 0b0000000000100000, VK_ACCESS_SHADER_READ_BIT , D3D12_BARRIER_ACCESS_UNORDERED_ACCESS) \
DEFINITION(ShaderWrite, 0b0000000001000000, VK_ACCESS_SHADER_WRITE_BIT , D3D12_BARRIER_ACCESS_UNORDERED_ACCESS) \
DEFINITION(ColorAttachmentRead, 0b0000000010000000, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT , D3D12_BARRIER_ACCESS_RENDER_TARGET) \
DEFINITION(ColorAttachmentWrite, 0b0000000100000000, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT , D3D12_BARRIER_ACCESS_RENDER_TARGET) \
DEFINITION(DepthStencilRead, 0b0000001000000000, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT , D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ) \
DEFINITION(DepthStencilWrite, 0b0000010000000000, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT , D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE) \
DEFINITION(TransferRead, 0b0000100000000000, VK_ACCESS_TRANSFER_READ_BIT , D3D12_BARRIER_ACCESS_COPY_SOURCE) \
DEFINITION(TransferWrite, 0b0001000000000000, VK_ACCESS_TRANSFER_WRITE_BIT , D3D12_BARRIER_ACCESS_COPY_DEST) \
DEFINITION(AllRead, 0b0010000000000000, VK_ACCESS_MEMORY_READ_BIT , D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT | D3D12_BARRIER_ACCESS_INDEX_BUFFER | D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_CONSTANT_BUFFER | D3D12_BARRIER_ACCESS_SHADER_RESOURCE | D3D12_BARRIER_ACCESS_UNORDERED_ACCESS | D3D12_BARRIER_ACCESS_RENDER_TARGET | D3D12_BARRIER_ACCESS_COPY_SOURCE | D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ) \
DEFINITION(AllWrite, 0b0100000000000000, VK_ACCESS_MEMORY_WRITE_BIT , D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT | D3D12_BARRIER_ACCESS_INDEX_BUFFER | D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_CONSTANT_BUFFER | D3D12_BARRIER_ACCESS_SHADER_RESOURCE | D3D12_BARRIER_ACCESS_UNORDERED_ACCESS | D3D12_BARRIER_ACCESS_RENDER_TARGET | D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE | D3D12_BARRIER_ACCESS_COPY_DEST) \

#include "Texture.h"
#include "Buffer.h"

namespace slag
{
    enum BarrierAccess
    {
#define DEFINITION(slagName, slagValue, vulkanName, dirextXName) slagName = slagValue,
        MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    };
    inline BarrierAccess operator|(BarrierAccess a, BarrierAccess b)
    {
        return static_cast<BarrierAccess>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
    }

    inline BarrierAccess operator|=(BarrierAccess& a, BarrierAccess b)
    {
        a = a|b;
        return a;
    }

    inline BarrierAccess operator&(BarrierAccess a, BarrierAccess b)
    {
        return static_cast<BarrierAccess>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
    }

    inline BarrierAccess operator&=(BarrierAccess& a, BarrierAccess b)
    {
        a = a&b;
        return a;
    }

    inline BarrierAccess operator~(BarrierAccess a)
    {
        return static_cast<BarrierAccess>(~static_cast<unsigned int>(a));
    }

    struct ImageBarrier
    {
        Texture* texture = nullptr;
        Texture::Layout oldLayout = Texture::UNDEFINED;
        Texture::Layout newLayout = Texture::UNDEFINED;
        BarrierAccess accessBefore = BarrierAccess::AllRead | BarrierAccess::AllWrite;
        BarrierAccess accessAfter = BarrierAccess::AllRead | BarrierAccess::AllWrite;
    };

    struct BufferBarrier
    {
        Buffer* buffer = nullptr;
        size_t offset = 0;
        ///Size of section of barrier to require barrier, 0 = entire size of the buffer
        size_t size = 0;
        BarrierAccess accessBefore = BarrierAccess::AllRead | BarrierAccess::AllWrite;
        BarrierAccess accessAfter = BarrierAccess::AllRead | BarrierAccess::AllWrite;
    };
}

#endif //SLAG_GPUMEMORYBARRIERS_H
