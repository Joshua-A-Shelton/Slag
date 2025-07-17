#ifndef SLAG_GPUBARRIERS_H
#define SLAG_GPUBARRIERS_H

#define SLAG_MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION) \
DEFINITION(NONE, 0b0000000000000000, VK_ACCESS_NONE , D3D12_BARRIER_ACCESS_NO_ACCESS) \
DEFINITION(INDIRECT_COMMAND, 0b0000000000000001, VK_ACCESS_INDIRECT_COMMAND_READ_BIT , D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT) \
DEFINITION(INDEX, 0b0000000000000010, VK_ACCESS_INDEX_READ_BIT , D3D12_BARRIER_ACCESS_INDEX_BUFFER) \
DEFINITION(VERTEX_ATTRIBUTE, 0b0000000000000100, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT , D3D12_BARRIER_ACCESS_VERTEX_BUFFER) \
DEFINITION(UNIFORM, 0b0000000000001000, VK_ACCESS_UNIFORM_READ_BIT , D3D12_BARRIER_ACCESS_CONSTANT_BUFFER) \
DEFINITION(INPUT_ATTACHMENT, 0b0000000000010000, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT , D3D12_BARRIER_ACCESS_SHADER_RESOURCE) \
DEFINITION(SHADER_READ, 0b0000000000100000, VK_ACCESS_SHADER_READ_BIT , D3D12_BARRIER_ACCESS_UNORDERED_ACCESS) \
DEFINITION(SHADER_WRITE, 0b0000000001000000, VK_ACCESS_SHADER_WRITE_BIT , D3D12_BARRIER_ACCESS_UNORDERED_ACCESS) \
DEFINITION(COLOR_ATTACHMENT_READ, 0b0000000010000000, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT , D3D12_BARRIER_ACCESS_RENDER_TARGET) \
DEFINITION(COLOR_ATTACHMENT_WRITE, 0b0000000100000000, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT , D3D12_BARRIER_ACCESS_RENDER_TARGET) \
DEFINITION(DEPTH_STENCIL_READ, 0b0000001000000000, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT , D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ) \
DEFINITION(DEPTH_STENCIL_WRITE, 0b0000010000000000, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT , D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE) \
DEFINITION(TRANSFER_READ, 0b0000100000000000, VK_ACCESS_TRANSFER_READ_BIT , D3D12_BARRIER_ACCESS_COPY_SOURCE) \
DEFINITION(TRANSFER_WRITE, 0b0001000000000000, VK_ACCESS_TRANSFER_WRITE_BIT , D3D12_BARRIER_ACCESS_COPY_DEST) \
DEFINITION(CLEAR, 0b0010000000000000, VK_ACCESS_TRANSFER_WRITE_BIT , D3D12_BARRIER_ACCESS_UNORDERED_ACCESS) \
DEFINITION(BLIT_READ, 0b0100000000000000, VK_ACCESS_TRANSFER_READ_BIT , D3D12_BARRIER_ACCESS_UNORDERED_ACCESS) \
DEFINITION(BLIT_WRITE, 0b1000000000000000, VK_ACCESS_TRANSFER_WRITE_BIT , D3D12_BARRIER_ACCESS_UNORDERED_ACCESS) \

#define SLAG_MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION) \
DEFINITION(NONE,0, VK_PIPELINE_STAGE_NONE, D3D12_BARRIER_SYNC_NONE) \
DEFINITION(DRAW_INDIRECT,1, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,D3D12_BARRIER_SYNC_EXECUTE_INDIRECT)\
DEFINITION(INDEX_INPUT,1<<1, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,D3D12_BARRIER_SYNC_INDEX_INPUT )\
DEFINITION(VERTEX_SHADER,1<<2, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,D3D12_BARRIER_SYNC_VERTEX_SHADING )\
DEFINITION(TESSELATION_CONTROL,1<<3, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,D3D12_BARRIER_SYNC_NON_PIXEL_SHADING )\
DEFINITION(TESSELATION_EVALUATION,1<<4, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,D3D12_BARRIER_SYNC_NON_PIXEL_SHADING )\
DEFINITION(GEOMETRY_SHADER,1<<5, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,D3D12_BARRIER_SYNC_NON_PIXEL_SHADING ) \
DEFINITION(FRAGMENT_SHADER,1<<6, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,D3D12_BARRIER_SYNC_PIXEL_SHADING) \
DEFINITION(EARLY_DEPTH_STENCIL,1<<7, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,D3D12_BARRIER_SYNC_DEPTH_STENCIL) \
DEFINITION(LATE_DEPTH_STENCIL,1<<8, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,D3D12_BARRIER_SYNC_DEPTH_STENCIL) \
DEFINITION(COLOR_ATTACHMENT,1<<9, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,D3D12_BARRIER_SYNC_RENDER_TARGET ) \
DEFINITION(COMPUTE_SHADER,1<<10, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,D3D12_BARRIER_SYNC_COMPUTE_SHADING) \
DEFINITION(TRANSFER,1<<11, VK_PIPELINE_STAGE_TRANSFER_BIT,D3D12_BARRIER_SYNC_COPY)                     \
DEFINITION(ACCELERATION_STRUCTURE_BUILD,1<<12, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR ,D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE)\
DEFINITION(RAY_TRACING,1<<13, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,D3D12_BARRIER_SYNC_RAYTRACING)\
DEFINITION(BLIT,1<<14, VK_PIPELINE_STAGE_TRANSFER_BIT,D3D12_BARRIER_SYNC_COMPUTE_SHADING) \
DEFINITION(CLEAR_COLOR,1<<15, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, D3D12_BARRIER_SYNC_RENDER_TARGET)\
DEFINITION(CLEAR_DEPTH,1<<16, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, D3D12_BARRIER_SYNC_DEPTH_STENCIL)\
DEFINITION(ALL_GRAPHICS,1<<17, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,D3D12_BARRIER_SYNC_ALL_SHADING) \
DEFINITION(ALL_COMMANDS,1<<18, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,D3D12_BARRIER_SYNC_ALL) \


#define SLAG_TEXTURE_LAYOUT_DEFINITIONS(DEFINITION)\
DEFINITION(UNDEFINED,VK_IMAGE_LAYOUT_UNDEFINED,D3D12_BARRIER_LAYOUT_UNDEFINED,D3D12_RESOURCE_STATE_COMMON) \
DEFINITION(RENDER_TARGET,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,D3D12_BARRIER_LAYOUT_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET ) \
DEFINITION(GENERAL,VK_IMAGE_LAYOUT_GENERAL,D3D12_BARRIER_LAYOUT_COMMON,D3D12_RESOURCE_STATE_COMMON)        \
DEFINITION(UNORDERED,VK_IMAGE_LAYOUT_GENERAL,D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS)        \
DEFINITION(DEPTH_TARGET_READ_ONLY,VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,D3D12_RESOURCE_STATE_DEPTH_READ ) \
DEFINITION(DEPTH_TARGET,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE,D3D12_RESOURCE_STATE_DEPTH_WRITE ) \
DEFINITION(SHADER_RESOURCE,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE) \
DEFINITION(TRANSFER_DESTINATION,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,D3D12_BARRIER_LAYOUT_COPY_DEST,D3D12_RESOURCE_STATE_COPY_DEST) \
DEFINITION(TRANSFER_SOURCE,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,D3D12_BARRIER_LAYOUT_COPY_SOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE) \
DEFINITION(RESOLVE_DESTINATION,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,D3D12_BARRIER_LAYOUT_COPY_DEST,D3D12_RESOURCE_STATE_RESOLVE_DEST) \
DEFINITION(RESOLVE_SOURCE,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,D3D12_BARRIER_LAYOUT_COPY_SOURCE,D3D12_RESOURCE_STATE_RESOLVE_SOURCE) \
DEFINITION(PRESENT,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,D3D12_BARRIER_LAYOUT_PRESENT,D3D12_RESOURCE_STATE_PRESENT ) \
DEFINITION(BLIT_SOURCE,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS ) \
DEFINITION(BLIT_DESTINATION,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS ) \

#include <cstdint>
#include <cstddef>

namespace slag
{
    class Texture;
    class Buffer;
    ///Bit flags that indicate memory usage for a gpu resource
    enum class BarrierAccessFlags
    {
#define DEFINITION(slagName, slagValue, vulkanName, directXName) slagName = slagValue,
        SLAG_MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    };

    inline BarrierAccessFlags operator|(BarrierAccessFlags a, BarrierAccessFlags b)
    {
        return static_cast<BarrierAccessFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline BarrierAccessFlags operator&(BarrierAccessFlags a, BarrierAccessFlags b)
    {
        return static_cast<BarrierAccessFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline BarrierAccessFlags operator~(BarrierAccessFlags a)
    {
        return static_cast<BarrierAccessFlags>(~static_cast<uint32_t>(a));
    }

    inline BarrierAccessFlags operator|=(BarrierAccessFlags& a, BarrierAccessFlags b)
    {
        a = a | b;
        return a;
    }

    inline BarrierAccessFlags operator&=(BarrierAccessFlags& a, BarrierAccessFlags b)
    {
        a = a & b;
        return a;
    }
    ///Bit flags that indicate stages in GPU execution
    enum class PipelineStageFlags
    {
#define DEFINITION(slagName, slagValue, vulkanName, directXName) slagName = slagValue,
        SLAG_MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION
    };

    inline PipelineStageFlags operator|(PipelineStageFlags a, PipelineStageFlags b)
    {
        return static_cast<PipelineStageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline PipelineStageFlags operator&(PipelineStageFlags a, PipelineStageFlags b)
    {
        return static_cast<PipelineStageFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline PipelineStageFlags operator~(PipelineStageFlags a)
    {
        return static_cast<PipelineStageFlags>(~static_cast<uint32_t>(a));
    }

    inline PipelineStageFlags operator|=(PipelineStageFlags& a, PipelineStageFlags b)
    {
        a = a | b;
        return a;
    }

    inline PipelineStageFlags operator&=(PipelineStageFlags& a, PipelineStageFlags b)
    {
        a = a & b;
        return a;
    }

#ifdef SLAG_DISCREET_TEXTURE_LAYOUTS
    class TextureLayouts
    {
    public:
        ///Layouts for textures that are optimized for specific operations
        enum Layout
        {
#define DEFINITION(slagName, vulkanName, directXName) slagName,
            SLAG_TEXTURE_LAYOUT_DEFINITIONS(DEFINITION)
#undef DEFINITION
        };
    };

    struct TextureBarrierDiscreet
    {
        Texture* texture = nullptr;
        uint32_t baseLayer = 0;
        /// layerCount = 0 means all remaining layers
        uint32_t layerCount = 0;
        uint32_t baseMipLevel = 0;
        /// mipCount = 0 means all remaining mips
        uint32_t mipCount = 0;
        TextureLayouts::Layout oldLayout = TextureLayouts::UNDEFINED;
        TextureLayouts::Layout newLayout = TextureLayouts::UNDEFINED;
        ///Synchronize (flush from cache [cache->Memory]) this kind of memory before barrier executes
        BarrierAccessFlags accessBefore = BarrierAccessFlags::NONE;
        ///Synchronize (invalidate cache [cache<-Memory]) this kind of memory after barrier executes
        BarrierAccessFlags accessAfter = BarrierAccessFlags::NONE;
        ///Finish all work of this kind before barrier executes
        PipelineStageFlags syncBefore = PipelineStageFlags::NONE;
        ///Make all work of this kind wait until after barrier executes
        PipelineStageFlags syncAfter = PipelineStageFlags::NONE;
    };
#else
    struct TextureBarrier
    {
        Texture* texture = nullptr;
        uint32_t baseLayer = 0;
        /// layerCount = 0 means all remaining layers
        uint32_t layerCount = 0;
        uint32_t baseMipLevel = 0;
        /// mipCount = 0 means all remaining mips
        uint32_t mipCount = 0;
        ///Synchronize (flush from cache [cache->Memory]) this kind of memory before barrier executes
        BarrierAccessFlags accessBefore = BarrierAccessFlags::NONE;
        ///Synchronize (invalidate cache [cache<-Memory]) this kind of memory after barrier executes
        BarrierAccessFlags accessAfter = BarrierAccessFlags::NONE;
        ///Finish all work of this kind before barrier executes
        PipelineStageFlags syncBefore = PipelineStageFlags::NONE;
        ///Make all work of this kind wait until after barrier executes
        PipelineStageFlags syncAfter = PipelineStageFlags::NONE;
    };
#endif

    struct BufferBarrier
    {
        Buffer* buffer = nullptr;
        size_t offset = 0;
        ///Size of section of barrier to require barrier, 0 = entire size of the buffer
        size_t size = 0;
        ///Synchronize (flush from cache [cache->Memory]) this kind of memory before barrier executes
        BarrierAccessFlags accessBefore = BarrierAccessFlags::NONE;
        ///Synchronize (invalidate cache [cache<-Memory]) this kind of memory after barrier executes
        BarrierAccessFlags accessAfter = BarrierAccessFlags::NONE;
        ///Finish all work of this kind before barrier executes
        PipelineStageFlags syncBefore = PipelineStageFlags::ALL_COMMANDS;
        ///Make all work of this kind wait until after barrier executes
        PipelineStageFlags syncAfter = PipelineStageFlags::ALL_COMMANDS;
    };

    struct GlobalBarrier
    {
        ///Synchronize (flush from cache [cache->Memory]) this kind of memory before barrier executes
        BarrierAccessFlags accessBefore = BarrierAccessFlags::NONE;
        ///Synchronize (invalidate cache [cache<-Memory]) this kind of memory after barrier executes
        BarrierAccessFlags accessAfter = BarrierAccessFlags::NONE;
        ///Finish all work of this kind before barrier executes
        PipelineStageFlags syncBefore = PipelineStageFlags::ALL_COMMANDS;
        ///Make all work of this kind wait until after barrier executes
        PipelineStageFlags syncAfter = PipelineStageFlags::ALL_COMMANDS;
    };
} // slag

#endif //SLAG_GPUBARRIERS_H
