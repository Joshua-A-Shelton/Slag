#ifndef SLAG_GPUMEMORYBARRIERS_H
#define SLAG_GPUMEMORYBARRIERS_H

#define MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION) \
DEFINITION(NONE, VK_ACCESS_NONE , D3D12_BARRIER_ACCESS_NO_ACCESS) \
DEFINITION(INDIRECT_COMMAND, VK_ACCESS_INDIRECT_COMMAND_READ_BIT , D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT) \
DEFINITION(INDEX, VK_ACCESS_INDEX_READ_BIT , D3D12_BARRIER_ACCESS_INDEX_BUFFER) \
DEFINITION(VERTEX_ATTRIBUTE, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT , D3D12_BARRIER_ACCESS_VERTEX_BUFFER) \
DEFINITION(UNIFORM, VK_ACCESS_UNIFORM_READ_BIT , D3D12_BARRIER_ACCESS_CONSTANT_BUFFER) \
DEFINITION(INPUT_ATTACHMENT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT , D3D12_BARRIER_ACCESS_SHADER_RESOURCE) \
DEFINITION(SHADER_READ, VK_ACCESS_SHADER_READ_BIT , D3D12_BARRIER_ACCESS_UNORDERED_ACCESS) \
DEFINITION(SHADER_WRITE, VK_ACCESS_SHADER_WRITE_BIT , D3D12_BARRIER_ACCESS_UNORDERED_ACCESS) \
DEFINITION(COLOR_ATTACHMENT_READ, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT , D3D12_BARRIER_ACCESS_RENDER_TARGET) \
DEFINITION(COLOR_ATTACHMENT_WRITE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT , D3D12_BARRIER_ACCESS_RENDER_TARGET) \
DEFINITION(DEPTH_STENCIL_READ, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT , D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ) \
DEFINITION(DEPTH_STENCIL_WRITE, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT , D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE) \
DEFINITION(TRANSFER_READ, VK_ACCESS_TRANSFER_READ_BIT , D3D12_BARRIER_ACCESS_COPY_SOURCE) \
DEFINITION(TRANSFER_WRITE, VK_ACCESS_TRANSFER_WRITE_BIT , D3D12_BARRIER_ACCESS_COPY_DEST) \
DEFINITION(ALL_READ, VK_ACCESS_MEMORY_READ_BIT , D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT | D3D12_BARRIER_ACCESS_INDEX_BUFFER | D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_CONSTANT_BUFFER | D3D12_BARRIER_ACCESS_SHADER_RESOURCE | D3D12_BARRIER_ACCESS_UNORDERED_ACCESS | D3D12_BARRIER_ACCESS_RENDER_TARGET | D3D12_BARRIER_ACCESS_COPY_SOURCE | D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ) \
DEFINITION(ALL_WRITE, VK_ACCESS_MEMORY_WRITE_BIT , D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT | D3D12_BARRIER_ACCESS_INDEX_BUFFER | D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_CONSTANT_BUFFER | D3D12_BARRIER_ACCESS_SHADER_RESOURCE | D3D12_BARRIER_ACCESS_UNORDERED_ACCESS | D3D12_BARRIER_ACCESS_RENDER_TARGET | D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE | D3D12_BARRIER_ACCESS_COPY_DEST) \


#define MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION) \
DEFINITION(NONE, VK_PIPELINE_STAGE_NONE, D3D12_BARRIER_SYNC_NONE) \
DEFINITION(DRAW_INDIRECT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,D3D12_BARRIER_SYNC_EXECUTE_INDIRECT)\
DEFINITION(INDEX_INPUT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,D3D12_BARRIER_SYNC_INDEX_INPUT )\
DEFINITION(VERTEX_SHADER, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,D3D12_BARRIER_SYNC_VERTEX_SHADING )\
DEFINITION(TESSELATION_CONTROL, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,D3D12_BARRIER_SYNC_NON_PIXEL_SHADING )\
DEFINITION(TESSELATION_EVALUATION, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,D3D12_BARRIER_SYNC_NON_PIXEL_SHADING )\
DEFINITION(GEOMETRY_SHADER, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,D3D12_BARRIER_SYNC_NON_PIXEL_SHADING ) \
DEFINITION(FRAGMENT_SHADER, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,D3D12_BARRIER_SYNC_PIXEL_SHADING) \
DEFINITION(EARLY_DEPTH_STENCIL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,D3D12_BARRIER_SYNC_DEPTH_STENCIL) \
DEFINITION(LATE_DEPTH_STENCIL, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,D3D12_BARRIER_SYNC_DEPTH_STENCIL) \
DEFINITION(COLOR_ATTACHMENT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,D3D12_BARRIER_SYNC_RENDER_TARGET ) \
DEFINITION(COMPUTE_SHADER, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,D3D12_BARRIER_SYNC_COMPUTE_SHADING) \
DEFINITION(TRANSFER, VK_PIPELINE_STAGE_TRANSFER_BIT,D3D12_BARRIER_SYNC_COPY)                     \
DEFINITION(ACCELERATION_STRUCTURE_BUILD, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR ,D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE)\
DEFINITION(RAY_TRACING, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,D3D12_BARRIER_SYNC_RAYTRACING)\
DEFINITION(ALL_GRAPHICS, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,D3D12_BARRIER_SYNC_ALL_SHADING) \
DEFINITION(ALL_COMMANDS, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,D3D12_BARRIER_SYNC_ALL) \


#include "Texture.h"
#include "Buffer.h"

namespace slag
{
    class BarrierAccess
    {
    private:
        int _value;
        explicit BarrierAccess(int val){ _value=val;}
    public:
        friend class BarrierAccessFlags;
        BarrierAccess operator| (BarrierAccess b) const
        {
            return BarrierAccess(_value | b._value);
        }

        BarrierAccess& operator |=(BarrierAccess b)
        {
            _value = _value|b._value;
            return *this;
        }

        BarrierAccess operator&(BarrierAccess b) const
        {
            return BarrierAccess(_value & b._value);
        }

        BarrierAccess& operator&=(BarrierAccess b)
        {
            _value = _value&b._value;
            return *this;
        }

        BarrierAccess operator~() const
        {
            return BarrierAccess(~_value);
        }
    };
    class BarrierAccessFlags
    {
#define DEFINITION(slagName, vulkanName, directXName) inline static BarrierAccess _##slagName = BarrierAccess(0);
        MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    public:
#define DEFINITION(slagName, vulkanName, directXName) inline static const BarrierAccess& slagName = (_##slagName); /***Only use this if you *REALLY* know what you're doing, will override underlying library value for flag*/static void set##slagName##Value(int val){_##slagName._value = val;}
        MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    };

    class PipelineStages
    {
    private:
        int _value = 0;
        PipelineStages(int val){ _value=val;}
    public:
        friend class PipelineStageFlags;
        PipelineStages operator| (PipelineStages b) const
        {
            return PipelineStages(_value | b._value);
        }

        PipelineStages& operator |=(PipelineStages b)
        {
            _value = _value|b._value;
            return *this;
        }

        PipelineStages operator&(PipelineStages b) const
        {
            return PipelineStages(_value & b._value);
        }

        PipelineStages& operator&=(PipelineStages b)
        {
            _value = _value&b._value;
            return *this;
        }

        PipelineStages operator~() const
        {
            return PipelineStages(~_value);
        }
    };

    class PipelineStageFlags
    {
#define DEFINITION(slagName, vulkanName, directXName) inline static PipelineStages _##slagName = 0;
        MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION
    public:
#define DEFINITION(slagName, vulkanName, directXName) inline static PipelineStages& slagName = _##slagName; /***Only use this if you *REALLY* know what you're doing, will override underlying library value for flag*/ static void set##slagName##Value(int val){_##slagName._value = val;}
        MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION
    };

    struct ImageBarrier
    {
        Texture* texture = nullptr;
        Texture::Layout oldLayout = Texture::UNDEFINED;
        Texture::Layout newLayout = Texture::UNDEFINED;
        ///Synchronize (flush from cache [cache->Memory]) this kind of memory before barrier executes
        BarrierAccess accessBefore = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
        ///Synchronize (invalidate cache [cache<-Memory]) this kind of memory after barrier executes
        BarrierAccess accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
        ///Finish all work of this kind before barrier executes
        PipelineStages syncBefore = PipelineStageFlags::ALL_COMMANDS;
        ///Make all work of this kind wait until after barrier executes
        PipelineStages syncAfter = PipelineStageFlags::ALL_COMMANDS;
    };

    struct BufferBarrier
    {
        Buffer* buffer = nullptr;
        size_t offset = 0;
        ///Size of section of barrier to require barrier, 0 = entire size of the buffer
        size_t size = 0;
        ///Synchronize (flush from cache [cache->Memory]) this kind of memory before barrier executes
        BarrierAccess accessBefore = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
        ///Synchronize (invalidate cache [cache<-Memory]) this kind of memory after barrier executes
        BarrierAccess accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
        ///Finish all work of this kind before barrier executes
        PipelineStages syncBefore = PipelineStageFlags::ALL_COMMANDS;
        ///Make all work of this kind wait until after barrier executes
        PipelineStages syncAfter = PipelineStageFlags::ALL_COMMANDS;
    };

    struct GPUMemoryBarrier
    {
        ///Synchronize (flush from cache [cache->Memory]) this kind of memory before barrier executes
        BarrierAccess accessBefore = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
        ///Synchronize (invalidate cache [cache<-Memory]) this kind of memory after barrier executes
        BarrierAccess accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
        ///Finish all work of this kind before barrier executes
        PipelineStages syncBefore = PipelineStageFlags::ALL_COMMANDS;
        ///Make all work of this kind wait until after barrier executes
        PipelineStages syncAfter = PipelineStageFlags::ALL_COMMANDS;
    };
}

#endif //SLAG_GPUMEMORYBARRIERS_H
