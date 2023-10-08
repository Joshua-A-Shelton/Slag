#ifndef SLAG_SLAGLIB_H
#define SLAG_SLAGLIB_H
#include "CommandBuffer.h"
#include "Frame.h"
#include "GraphicsCard.h"
#include "PixelFormat.h"
#include "PlatformData.h"
#include "Swapchain.h"
#include "Texture.h"
#include "UniformDescriptor.h"
#include "UniformDescriptorSet.h"
#include "UniformSet.h"

namespace slag
{

    enum BackEnd
    {
        VULKAN,
        DX12
    };
    struct SlagInitDetails
    {
        BackEnd backend = VULKAN;
    };
    class SlagLib
    {
    public:
        static bool initialize(SlagInitDetails initDetails);
        static void cleanup();
        static BackEnd usingBackEnd();
    };

} // slag

#endif //SLAG_SLAGLIB_H
