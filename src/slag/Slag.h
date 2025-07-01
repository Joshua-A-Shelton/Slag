#ifndef SLAG_SLAG_H
#define SLAG_SLAG_H

#include "core/CommandBuffer.h"
#include "core/GPUQueue.h"
#include "core/GraphicsCard.h"
#include "core/Pixels.h"
#include "core/Semaphore.h"
#include "core/SwapChain.h"
#include "core/Texture.h"
namespace slag
{
    enum GraphicsBackend
    {
        DEFAULT_GRAPHICS_BACKEND = 0,
        VULKAN_GRAPHICS_BACKEND,
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
        ///Function pointer that forwards error messages from the underlying API, nullptr disables error handling
        void(* slagDebugHandler)(const std::string& message, DebugLevel debugLevel, int32_t messageID)=nullptr;
        ///Function pointer that evaluates graphics cards when deciding which one to pick, returning true means we should pick a over b, nullptr uses default sorting
        bool(* graphicsCardEvaluationHandler)(const GraphicsCard* a, const GraphicsCard* b)=nullptr;

    };

    bool initialize(const SlagInitInfo& initInfo={});
    void cleanup();
    GraphicsCard* graphicsCard();
}

#endif //SLAG_SLAG_H
