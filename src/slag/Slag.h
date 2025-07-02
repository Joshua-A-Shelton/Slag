#ifndef SLAG_SLAG_H
#define SLAG_SLAG_H

#include "core/Buffer.h"
#include "core/Clear.h"
#include "core/CommandBuffer.h"
#include "core/Dimensions.h"
#include "core/Frame.h"
#include "core/FrameResources.h"
#include "core/GPUBarriers.h"
#include "core/GPUQueue.h"
#include "core/GraphicsCard.h"
#include "core/ICommandBuffer.h"
#include "core/Pixels.h"
#include "core/PlatformData.h"
#include "core/Semaphore.h"
#include "core/SwapChain.h"
#include "core/Texture.h"

namespace slag
{
    enum GraphicsBackend
    {
        DEFAULT_GRAPHICS_BACKEND = 0,
        VULKAN_GRAPHICS_BACKEND,
        CUSTOM_GRAPHICS_BACKEND
    };
    struct SlagInitInfo
    {
        enum DebugLevel
        {
            SLAG_ERROR,
            SLAG_WARNING,
            SLAG_INFO
        };
        GraphicsBackend graphicsBackend = DEFAULT_GRAPHICS_BACKEND;
        ///Pointer to object that implements slag::Backend if you need to provide your own instead of one of the provided ones
        void* customBackend = nullptr;
        ///Function pointer that forwards error messages from the underlying API, nullptr disables error handling
        void(* slagDebugHandler)(const std::string& message, DebugLevel debugLevel, int32_t messageID)=nullptr;
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
    GraphicsCard* graphicsCard();
}

#endif //SLAG_SLAG_H
