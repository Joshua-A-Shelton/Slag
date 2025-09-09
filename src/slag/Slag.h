#ifndef SLAG_SLAG_H
#define SLAG_SLAG_H

#include "core/Buffer.h"
#include "core/BufferView.h"
#include "core/Clear.h"
#include "core/CommandBuffer.h"
#include "core/Descriptor.h"
#include "core/DescriptorBundle.h"
#include "core/DescriptorGroup.h"
#include "core/DescriptorPool.h"
#include "core/Dimensions.h"
#include "core/Frame.h"
#include "core/FrameResources.h"
#include "core/GPUBarriers.h"
#include "core/GPUQueue.h"
#include "core/GraphicsCard.h"
#include "core/GraphicsTypes.h"
#include "core/ICommandBuffer.h"
#include "core/Pixels.h"
#include "core/PlatformData.h"
#include "core/Sampler.h"
#include "core/Semaphore.h"
#include "core/ShaderPipeline.h"
#include "core/SwapChain.h"
#include "core/Texture.h"
#include "core/VertexDescription.h"

namespace slag
{
    enum class GraphicsBackend
    {
        DEFAULT_GRAPHICS_BACKEND = 0,
        VULKAN_GRAPHICS_BACKEND,
        CUSTOM_GRAPHICS_BACKEND
    };
    enum class SlagDebugLevel
    {
        SLAG_ERROR,
        SLAG_WARNING,
        SLAG_INFO
    };
    struct SlagInitInfo
    {
        GraphicsBackend graphicsBackend = GraphicsBackend::DEFAULT_GRAPHICS_BACKEND;
        ///Pointer to object that implements slag::Backend if you need to provide your own instead of one of the provided ones
        void* customBackend = nullptr;
        ///Function pointer that forwards error messages from the underlying API, nullptr disables error handling
        void(* slagDebugHandler)(const std::string& message, SlagDebugLevel debugLevel, int32_t messageID)=nullptr;
        ///Function pointer that evaluates graphics cards when deciding which one to pick, returning true means we should pick a over b, nullptr uses default sorting
        bool(* graphicsCardEvaluationHandler)(const GraphicsCard* a, const GraphicsCard* b)=nullptr;

    };

    enum SlagInitializationResult
    {
        SLAG_INITIALIZATION_SUCCESS,
        SLAG_BACKEND_NOT_AVAILABLE,
        SLAG_NO_GRAPHICS_CARDS
    };

    SlagInitializationResult initialize(const SlagInitInfo& initInfo={});
    void cleanup();
    GraphicsCard* slagGraphicsCard();
}

#endif //SLAG_SLAG_H
