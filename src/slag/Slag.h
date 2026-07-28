#ifndef SLAG_SLAG_H
#define SLAG_SLAG_H

#include "core/Attachment.h"
#include "core/Barriers.h"
#include "core/Buffer.h"
#include "core/Clear.h"
#include "core/CommandBuffer.h"
#include "core/Defragmentation.h"
#include "core/Dimensions.h"
#include "core/Frame.h"
#include "core/GraphicsCard.h"
#include "core/IBackend.h"
#include "core/IBackend.h"
#include "core/ICommandBuffer.h"
#include "core/PixelFormatProperties.h"
#include "core/Pixels.h"
#include "core/PlatformData.h"
#include "core/ResourceDescriptorHeap.h"
#include "core/Sampler.h"
#include "core/SamplerDescriptorHeap.h"
#include "core/Semaphore.h"
#include "core/ShaderCode.h"
#include "core/ShaderPipeline.h"
#include "core/SubmissionQueue.h"
#include "core/SwapChain.h"
#include "core/Texture.h"
#include "core/VertexDescription.h"


namespace slag
{
    enum class DebugLevel
    {
        ERROR,
        WARNING,
        INFO
    };

    struct InitializationData
    {
        ///Graphics backend to use, or unknown to use the platform default
        BackendAPI backend = BackendAPI::UNKNOWN;
        ///Pointer to custom backend to use if backend was set to custom
        IBackend* customBackend = nullptr;
        ///Function pointer that will receive debug messages from the underlying API, nullptr disables debug messages
        void(*debugHandler)(const std::string& message,DebugLevel debugLevel, int32_t messageID)=nullptr;
    };


    class Slag
    {
    public:
        static SlagInitializationResult initialize(const InitializationData& initData);
        static void cleanup();
        static IBackend* backend();
    };
} // slag

#endif //SLAG_SLAG_H
