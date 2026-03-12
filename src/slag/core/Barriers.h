#ifndef SLAG_BARRIERS_H
#define SLAG_BARRIERS_H
#include <cstdint>

namespace slag
{
    class Buffer;
    class Texture;
    ///Represent different caches in memory that may be read/written to that needs to be flushed or invalidated
    enum class MemoryCaches: uint32_t
    {
        NONE                        = 0,
        INDIRECT_COMMAND_READ       = 1,
        INDEX_READ                  = 1 << 1,
        VERTEX_ATTRIBUTE_READ       = 1 << 2,
        UNIFORM_READ                = 1 << 3,
        COLOR_TARGET                = 1 << 5,
        DEPTH_TARGET_READ           = 1 << 6,
        DEPTH_TARGET_WRITE          = 1 << 7,
        SHADER_SAMPLED_READ         = 1 << 8,
        SHADER_UNORDERED_ACCESS     = 1 << 9,
        BLIT_READ                   = 1 << 10,
        BLIT_WRITE                  = 1 << 11,
        COPY_READ                   = 1 << 12,
        COPY_WRITE                  = 1 << 13,
        RESOLVE_READ                = 1 << 14,
        RESOLVE_WRITE               = 1 << 15,
        CLEAR                       = 1 << 16,
    };
    ///Represents different stages of a shader pipeline's execution
    enum class SyncStages: uint32_t
    {
        NONE                            = 0,
        ALL                             = 1,
        ALL_GRAPHICS                    = 1 << 1,
        INDEX_INPUT                     = 1 << 2,
        VERTEX_SHADER                   = 1 << 3,
        FRAGMENT_SHADER                 = 1 << 4,
        DEPTH_STENCIL_TARGET_OUTPUT     = 1 << 5,
        COLOR_TARGET_OUTPUT             = 1 << 6,
        COMPUTE_SHADER                  = 1 << 7,
        RAYTRACING_SHADER               = 1 << 8,
        COPY                            = 1 << 9,
        RESOLVE                         = 1 << 10,
        EXECUTE_INDIRECT                = 1 << 11,
        CLEAR                           = 1 << 12,
        VIDEO_DECODE                    = 1 << 13,
        VIDEO_ENCODE                    = 1 << 14,
        BUILD_ACCELERATION_STRUCTURE    = 1 << 15,
        COPY_ACCELERATION_STRUCTURE     = 1 << 16,
    };

    inline MemoryCaches operator|(MemoryCaches a, MemoryCaches b)
    {
        return static_cast<MemoryCaches>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline MemoryCaches operator&(MemoryCaches a, MemoryCaches b)
    {
        return static_cast<MemoryCaches>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline MemoryCaches operator^(MemoryCaches a, MemoryCaches b)
    {
        return static_cast<MemoryCaches>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
    }
    
    inline MemoryCaches operator~(MemoryCaches a)
    {
        return static_cast<MemoryCaches>(~static_cast<uint32_t>(a));
    }

    inline MemoryCaches operator|=(MemoryCaches a, MemoryCaches b)
    {
        a = a|b;
        return a;
    }

    inline MemoryCaches operator&=(MemoryCaches a, MemoryCaches b)
    {
        a = a&b;
        return a;
    }

    inline MemoryCaches operator^=(MemoryCaches a, MemoryCaches b)
    {
        a = a^b;
        return a;
    }

    inline SyncStages operator|(SyncStages a, SyncStages b)
    {
        return static_cast<SyncStages>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline SyncStages operator&(SyncStages a, SyncStages b)
    {
        return static_cast<SyncStages>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline SyncStages operator^(SyncStages a, SyncStages b)
    {
        return static_cast<SyncStages>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
    }
    
    inline SyncStages operator~(SyncStages a)
    {
        return static_cast<SyncStages>(~static_cast<uint32_t>(a));
    }

    inline SyncStages operator|=(SyncStages a, SyncStages b)
    {
        a = a|b;
        return a;
    }

    inline SyncStages operator&=(SyncStages a, SyncStages b)
    {
        a = a&b;
        return a;
    }

    inline SyncStages operator^=(SyncStages a, SyncStages b)
    {
        a = a^b;
        return a;
    }


    ///Barrier that applies to everything being executed in a command buffer
    struct GlobalBarrier
    {
        ///Finish all work of this kind before the barrier executes
        SyncStages syncBefore = SyncStages::ALL;
        ///Make all work of this kind wait until after the barrier executes
        SyncStages syncAfter  = SyncStages::ALL;
        ///Cache memory to write back to main memory
        MemoryCaches flush = MemoryCaches::NONE;
        ///Data to pull from main memory back into cache memory
        MemoryCaches invalidate = MemoryCaches::NONE;
    };
    ///Barrier that synchronizes buffer memory in execution of command buffer
    struct BufferBarrier
    {
        ///Buffer whose memory needs to be synchronized
        Buffer* buffer = nullptr;
        ///Offset into the buffer to synchronize
        uint64_t offset = 0;
        ///Length of the buffer data to synchronize (0 means entire length of buffer)
        uint64_t length = 0;
        ///Finish all work of this kind before the barrier executes
        SyncStages syncBefore = SyncStages::ALL;
        ///Make all work of this kind wait until after the barrier executes
        SyncStages syncAfter  = SyncStages::ALL;
        ///Cache memory to write back to main memory
        MemoryCaches flush = MemoryCaches::NONE;
        ///Data to pull from main memory back into cache memory
        MemoryCaches invalidate = MemoryCaches::NONE;
    };
    ///Barrier that synchronizes texture memory in execution of command buffer
    struct TextureBarrier
    {
        ///Texture whose memory needs to be synchronized
        Texture* texture = nullptr;
        ///Index into texture array of first texture to have its memory synchronized
        uint32_t baseLayer = 0;
        ///Number of textures in the array to have their memory synchronized
        uint32_t layerCount = 0;
        ///First mip level of the texture to have its memory synchronized
        uint32_t baseMipLevel = 0;
        ///Number of mip levels to have their memory synchronized
        uint32_t mipCount = 0;
        ///Finish all work of this kind before the barrier executes
        SyncStages syncBefore = SyncStages::ALL;
        ///Make all work of this kind wait until after the barrier executes
        SyncStages syncAfter  = SyncStages::ALL;
        ///Cache memory to write back to main memory
        MemoryCaches flush = MemoryCaches::NONE;
        ///Data to pull from main memory back into cache memory
        MemoryCaches invalidate = MemoryCaches::NONE;
    };
}
#endif //SLAG_BARRIERS_H