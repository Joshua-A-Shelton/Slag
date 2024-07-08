#ifndef SLAG_SLAGLIB_H
#define SLAG_SLAGLIB_H
#include "Attachment.h"
#include "Buffer.h"
#include "ClearValue.h"
#include "CommandBuffer.h"
#include "Frame.h"
#include "FramebufferDescription.h"
#include "FrameResourceDescriptions.h"
#include "GPUMemoryBarriers.h"
#include "GPUTypes.h"
#include "GraphicsCard.h"
#include "PixelFormat.h"
#include "PlatformData.h"
#include "PushConstantRange.h"
#include "Rectangle.h"
#include "Shader.h"
#include "Swapchain.h"
#include "Texture.h"
#include "TextureSampler.h"
#include "Uniform.h"
#include "UniformBuffer.h"
#include "UniformDescriptor.h"
#include "UniformSet.h"
#include "UniformSetDataAllocator.h"
#include "Viewport.h"
#include "VertexDescription.h"

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
        static GraphicsCard* graphicsCard();
    };

} // slag

#endif //SLAG_SLAGLIB_H
