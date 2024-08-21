#ifndef SLAG_SLAGLIB_H
#define SLAG_SLAGLIB_H

#include "Color.h"
#include "CommandBuffer.h"
#include "GpuQueue.h"
#include "GraphicsCard.h"
#include "Pipeline.h"
#include "Pixel.h"
#include "Semaphore.h"
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
