#ifndef SLAG_BARRIERS_H
#define SLAG_BARRIERS_H
#include <cstdint>

namespace slag
{
    class Buffer;
    class Texture;
    ///Represent different caches in memory that may be read/written to that needs to be flushed or invalidated
    enum class MemoryCache: uint32_t
    {
        NONE                        = 0,
        INDIRECT_COMMAND_READ       = 1,
        INDEX_READ                  = 1 << 1,
        VERTEX_ATTRIBUTE_READ       = 1 << 2,
        UNIFORM_READ                = 1 << 3,
        COLOR_TARGET_READ           = 1 << 4,
        COLOR_TARGET_WRITE          = 1 << 5,
        DEPTH_TARGET_READ           = 1 << 6,
        DEPTH_TARGET_WRITE          = 1 << 7,
        SHADER_SAMPLED_READ         = 1 << 8,
        SHADER_UNORDERED_READ       = 1 << 9,
        SHADER_UNORDERED_WRITE      = 1 << 10,
        BLIT_READ                   = 1 << 11,
        BLIT_WRITE                  = 1 << 12,
        COPY_READ                   = 1 << 13,
        COPY_WRITE                  = 1 << 14,
        CLEAR                       = 1 << 15,
    };
    ///Represents different stages of a shader pipeline's execution
    enum class SyncStage: uint32_t
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
        VIDEO_PROCESS                   = 1 << 14,
        VIDEO_ENCODE                    = 1 << 15,
        BUILD_ACCELERATION_STRUCTURE    = 1 << 16,
        COPY_ACCELERATION_STRUCTURE     = 1 << 17,
    };

    inline MemoryCache operator|(MemoryCache a, MemoryCache b)
    {
        return static_cast<MemoryCache>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline MemoryCache operator&(MemoryCache a, MemoryCache b)
    {
        return static_cast<MemoryCache>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline MemoryCache operator^(MemoryCache a, MemoryCache b)
    {
        return static_cast<MemoryCache>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
    }
    
    inline MemoryCache operator~(MemoryCache a)
    {
        return static_cast<MemoryCache>(~static_cast<uint32_t>(a));
    }

    inline MemoryCache operator|=(MemoryCache a, MemoryCache b)
    {
        a = a|b;
        return a;
    }

    inline MemoryCache operator&=(MemoryCache a, MemoryCache b)
    {
        a = a&b;
        return a;
    }

    inline MemoryCache operator^=(MemoryCache a, MemoryCache b)
    {
        a = a^b;
        return a;
    }

    inline SyncStage operator|(SyncStage a, SyncStage b)
    {
        return static_cast<SyncStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline SyncStage operator&(SyncStage a, SyncStage b)
    {
        return static_cast<SyncStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline SyncStage operator^(SyncStage a, SyncStage b)
    {
        return static_cast<SyncStage>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
    }
    
    inline SyncStage operator~(SyncStage a)
    {
        return static_cast<SyncStage>(~static_cast<uint32_t>(a));
    }

    inline SyncStage operator|=(SyncStage a, SyncStage b)
    {
        a = a|b;
        return a;
    }

    inline SyncStage operator&=(SyncStage a, SyncStage b)
    {
        a = a&b;
        return a;
    }

    inline SyncStage operator^=(SyncStage a, SyncStage b)
    {
        a = a^b;
        return a;
    }


    ///Barrier that applies to everything being executed in a command buffer
    struct GlobalBarrier
    {
        ///Finish all work of this kind before the barrier executes
        SyncStage syncBefore = SyncStage::ALL;
        ///Make all work of this kind wait until after the barrier executes
        SyncStage syncAfter  = SyncStage::ALL;
        ///Cache memory to write back to main memory
        MemoryCache flush = MemoryCache::NONE;
        ///Data to pull from main memory back into cache memory
        MemoryCache invalidate = MemoryCache::NONE;
    };
    ///Barrier that synchronizes buffer memory in execution of command buffer
    struct BufferBarrier
    {
        ///Buffer whose memory needs to be synchronized
        Buffer* buffer = nullptr;
        ///Offset into the buffer to synchronize
        size_t offset = 0;
        ///Length of the buffer data to synchronize (0 means entire length of buffer)
        size_t length = 0;
        ///Finish all work of this kind before the barrier executes
        SyncStage syncBefore = SyncStage::ALL;
        ///Make all work of this kind wait until after the barrier executes
        SyncStage syncAfter  = SyncStage::ALL;
        ///Cache memory to write back to main memory
        MemoryCache flush = MemoryCache::NONE;
        ///Data to pull from main memory back into cache memory
        MemoryCache invalidate = MemoryCache::NONE;
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
        SyncStage syncBefore = SyncStage::ALL;
        ///Make all work of this kind wait until after the barrier executes
        SyncStage syncAfter  = SyncStage::ALL;
        ///Cache memory to write back to main memory
        MemoryCache flush = MemoryCache::NONE;
        ///Data to pull from main memory back into cache memory
        MemoryCache invalidate = MemoryCache::NONE;
    };
}
#endif //SLAG_BARRIERS_H