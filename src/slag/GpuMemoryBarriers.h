#ifndef SLAG_GPUMEMORYBARRIERS_H
#define SLAG_GPUMEMORYBARRIERS_H

#define MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION) \
DEFINITION(AccessNone, 0b0000000000000000, VK_ACCESS_NONE , D3D12_BARRIER_ACCESS_NO_ACCESS) \
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


#define MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION) \
DEFINITION(StageNone, 0b0000000000000000, VK_PIPELINE_STAGE_2_NONE, D3D12_BARRIER_SYNC_NONE) \
DEFINITION(DrawIndirect, 0b0000000000000001, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,D3D12_BARRIER_SYNC_EXECUTE_INDIRECT)\
DEFINITION(InputAssembler, 0b0000000000000010, VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,D3D12_BARRIER_SYNC_INPUT_ASSEMBLER )\
DEFINITION(VertexShader, 0b0000000000000100, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,D3D12_BARRIER_SYNC_VERTEX_SHADING )\
DEFINITION(TesselationControl, 0b0000000000001000, VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,D3D12_BARRIER_SYNC_NON_PIXEL_SHADING )\
DEFINITION(TesselationEvaluation, 0b0000000000010000, VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,D3D12_BARRIER_SYNC_NON_PIXEL_SHADING )\
DEFINITION(GeometryShader, 0b0000000000100000, VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,D3D12_BARRIER_SYNC_NON_PIXEL_SHADING ) \
DEFINITION(FragmentShader, 0b0000000001000000, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,D3D12_BARRIER_SYNC_PIXEL_SHADING) \
DEFINITION(EarlyDepthStencil, 0b0000000010000000, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,D3D12_BARRIER_SYNC_DEPTH_STENCIL) \
DEFINITION(LateDepthStencil, 0b0000000100000000, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,D3D12_BARRIER_SYNC_DEPTH_STENCIL) \
DEFINITION(ColorAttachment, 0b0000001000000000, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,D3D12_BARRIER_SYNC_RENDER_TARGET ) \
DEFINITION(ComputeShader, 0b0000010000000000, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,D3D12_BARRIER_SYNC_COMPUTE_SHADING) \
DEFINITION(Transfer, 0b0000100000000000, VK_PIPELINE_STAGE_2_TRANSFER_BIT,D3D12_BARRIER_SYNC_COPY)                     \
DEFINITION(AccelerationStructureBuild, 0b0001000000000000, VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR ,D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE)\
DEFINITION(RayTracing, 0b0010000000000000, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,D3D12_BARRIER_SYNC_RAYTRACING)\
DEFINITION(AllGraphics, 0b0100000000000000, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,D3D12_BARRIER_SYNC_ALL_SHADING) \
DEFINITION(AllCommands, 0b1000000000000000, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,D3D12_BARRIER_SYNC_ALL) \


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

    enum PipelineStage
    {
#define DEFINITION(slagName, slagValue, vulkanName, dirextXName) slagName = slagValue,
        MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION
    };

    inline PipelineStage operator|(PipelineStage a, PipelineStage b)
    {
        return static_cast<PipelineStage>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
    }

    inline PipelineStage operator|=(PipelineStage& a, PipelineStage b)
    {
        a = a|b;
        return a;
    }

    inline PipelineStage operator&(PipelineStage a, PipelineStage b)
    {
        return static_cast<PipelineStage>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
    }

    inline PipelineStage operator&=(PipelineStage& a, PipelineStage b)
    {
        a = a&b;
        return a;
    }

    inline PipelineStage operator~(PipelineStage a)
    {
        return static_cast<PipelineStage>(~static_cast<unsigned int>(a));
    }

    struct ImageBarrier
    {
        Texture* texture = nullptr;
        Texture::Layout oldLayout = Texture::UNDEFINED;
        Texture::Layout newLayout = Texture::UNDEFINED;
        BarrierAccess accessBefore = BarrierAccess::AllRead | BarrierAccess::AllWrite;
        BarrierAccess accessAfter = BarrierAccess::AllRead | BarrierAccess::AllWrite;
        PipelineStage syncBefore = PipelineStage::AllCommands;
        PipelineStage syncAfter = PipelineStage::AllCommands;
    };

    struct BufferBarrier
    {
        Buffer* buffer = nullptr;
        size_t offset = 0;
        ///Size of section of barrier to require barrier, 0 = entire size of the buffer
        size_t size = 0;
        BarrierAccess accessBefore = BarrierAccess::AllRead | BarrierAccess::AllWrite;
        BarrierAccess accessAfter = BarrierAccess::AllRead | BarrierAccess::AllWrite;
        PipelineStage syncBefore = PipelineStage::AllCommands;
        PipelineStage syncAfter = PipelineStage::AllCommands;
    };

    struct GPUMemoryBarrier
    {
        BarrierAccess accessBefore = BarrierAccess::AllRead | BarrierAccess::AllWrite;
        BarrierAccess accessAfter = BarrierAccess::AllRead | BarrierAccess::AllWrite;
        PipelineStage syncBefore = PipelineStage::AllCommands;
        PipelineStage syncAfter = PipelineStage::AllCommands;
    };
}

#endif //SLAG_GPUMEMORYBARRIERS_H
