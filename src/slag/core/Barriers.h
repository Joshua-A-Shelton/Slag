#ifndef SLAG_BARRIERS_H
#define SLAG_BARRIERS_H
#include <cstdint>

namespace slag
{
    enum class BarrierAccess: uint32_t
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

    enum class SyncStage
    {
        NONE,
        ALL,
        ALL_GRAPHICS,
        INDEX_INPUT,
        VERTEX_SHADER,
        FRAGMENT_SHADER,
        DEPTH_STENCIL_TARGET_OUTPUT,
        COLOR_TARGET_OUTPUT,
        COMPUTE_SHADER,
        RAYTRACING_SHADER,
        COPY,
        RESOLVE,
        EXECUTE_INDIRECT,
        EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POST_BUILD,//rename
        CLEAR,
        VIDEO_DECODE,
        VIDEO_PROCESS,
        VIDEO_ENCODE,
        BUILD_RAYTRACING_STRUCTURE,//rename
        COPY_RAYTRACING_STRUCTURE,//rename
    };
}
#endif //SLAG_BARRIERS_H