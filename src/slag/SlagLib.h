#ifndef SLAG_SLAGLIB_H
#define SLAG_SLAGLIB_H

#include "Attachment.h"
#include "Buffer.h"
#include "Clear.h"
#include "Color.h"
#include "CommandBuffer.h"
#include "Descriptor.h"
#include "DescriptorGroup.h"
#include "DescriptorPool.h"
#include "Frame.h"
#include "FrameBufferDescription.h"
#include "GpuMemoryBarriers.h"
#include "GpuQueue.h"
#include "GraphicsCard.h"
#include "Operations.h"
#include "Pixel.h"
#include "PlatformData.h"
#include "Rectangle.h"
#include "Sampler.h"
#include "Semaphore.h"
#include "Shader.h"
#include "ShaderProperties.h"
#include "Swapchain.h"
#include "Texture.h"
#include "UniformBufferDescriptorLayout.h"
#include "VertexDescription.h"

namespace slag
{

    enum BackEnd
    {
        Vulkan,
        DirectX12
    };

    struct SlagInitDetails
    {
        enum DebugLevel
        {
            SLAG_ERROR,
            SLAG_WARNING,
            SLAG_MESSAGE
        };
        BackEnd backend = Vulkan;
        bool debug = false;
        void(* slagDebugHandler)(std::string& message, DebugLevel level, int32_t messageID) = nullptr;
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
