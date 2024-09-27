#ifndef SLAG_SLAGLIB_H
#define SLAG_SLAGLIB_H

#include "Buffer.h"
#include "Clear.h"
#include "Color.h"
#include "CommandBuffer.h"
#include "DescriptorPool.h"
#include "Frame.h"
#include "GpuMemoryBarriers.h"
#include "GpuQueue.h"
#include "GraphicsCard.h"
#include "Pixel.h"
#include "PlatformData.h"
#include "Rectangle.h"
#include "Semaphore.h"
#include "Swapchain.h"
#include "Texture.h"

namespace slag
{

    enum BackEnd
    {
        Vulkan,
        DirectX12
    };
    struct SlagInitDetails
    {
        BackEnd backend = Vulkan;
    };
    class SlagLib
    {
    public:
        static bool initialize(const SlagInitDetails& details);
        static void cleanup();
        static BackEnd usingBackEnd();
        static GraphicsCard* graphicsCard();
    };

} // slag

#endif //SLAG_SLAGLIB_H
