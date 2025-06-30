#ifndef SLAG_SLAG_H
#define SLAG_SLAG_H

#include "core/CommandBuffer.h"
#include "core/GPUQueue.h"
#include "core/GraphicsCard.h"
#include "core/Pixels.h"
#include "core/Semaphore.h"
#include "core/Texture.h"
namespace slag
{
    struct SlagInitInfo
    {
        enum DebugLevel
        {
            SLAG_ERROR,
            SLAG_WARNING,
            SLAG_INFO
        };
        ///Function pointer that forwards error messages from the underlying API, nullptr disables error handling
        void(* slagDebugHandler)(const std::string& message, DebugLevel debugLevel, int32_t messageID)=nullptr;
        ///Function pointer that evaluates graphics cards when deciding which one to pick, should return -1, 0, or 1, higher evaluated graphics cards are picked, nullptr enables default picking
        int(* graphicsCardEvaluationHandler)(const GraphicsCard* first, const GraphicsCard* second)=nullptr;

    };

    bool initialize(const SlagInitInfo& initInfo={});
    void cleanup();
    GraphicsCard* graphicsCard();
}

#endif //SLAG_SLAG_H
