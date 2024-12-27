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
#include "FrameResources.h"
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
    ///Backend API
    enum BackEnd
    {
        VULKAN,
        DIRECTX12
    };
    ///Details that sets up the rendering environment
    struct SlagInitDetails
    {
        ///The Severity of debug messages
        enum DebugLevel
        {
            SLAG_ERROR,
            SLAG_WARNING,
            SLAG_MESSAGE
        };
        ///Which Backend API to use
        BackEnd backend = VULKAN;
        ///Enable debug messages from the backend. Lower performance, but useful to see if the library is being used correctly
        bool debug = false;
        /**
         * Function pointer to method that will handle debug messages
         * @param message Text describing error
         * @param level Debug message severity
         * @param messageID Backend specific id for the type of message being sent
         */
        void(* slagDebugHandler)(std::string& message, DebugLevel level, int32_t messageID) = nullptr;
    };
    class SlagLib
    {
    public:
        /**
         * Initialize slag library. Must be called before any other function in slag library
         * @param details Information about the initialization of the library
         * @return Whether or not library was initialized successfully
         */
        static bool initialize(const SlagInitDetails& details);
        ///Cleans up resources associated with the slag library
        static void cleanup();
        ///What library backend slag is using
        static BackEnd usingBackEnd();
        ///Pointer to graphics card used for operations
        static GraphicsCard* graphicsCard();
    };

} // slag

#endif //SLAG_SLAGLIB_H
