#include "Slag.h"

#include <algorithm>
#include <slag/backends/Backend.h>
#include "utilities/SLAG_ASSERT.h"

#ifdef SLAG_VULKAN_BACKEND
#include <slag/backends/vulkan/VulkanBackend.h>
#endif

namespace slag
{
    std::unique_ptr<GraphicsCard> SLAG_CHOSEN_GRAPHICS_CARD=nullptr;

    bool initialize(const SlagInitInfo& initInfo)
    {
        SLAG_ASSERT(Backend::current()==nullptr && SLAG_CHOSEN_GRAPHICS_CARD==nullptr);

        switch (initInfo.graphicsBackend)
        {
        case GraphicsBackend::DEFAULT_GRAPHICS_BACKEND:
#ifdef SLAG_VULKAN_BACKEND
        case GraphicsBackend::VULKAN_GRAPHICS_BACKEND:
            Backend::_current = new vulkan::VulkanBackend(initInfo);
            break;
#endif
        default:
            return false;
        }
        if (Backend::current()==nullptr)
        {
            return false;
        }


        //set default graphics card
        auto cards = Backend::current()->getGraphicsCards();
        if (cards.empty())
        {
            delete Backend::_current;
            Backend::_current = nullptr;
            return false;
        }
        if (cards.size()==1)
        {
            SLAG_CHOSEN_GRAPHICS_CARD = std::move(cards.front());
        }
        else
        {
            if (initInfo.graphicsCardEvaluationHandler)
            {
                std::sort(cards.begin(), cards.end(),[&](const std::unique_ptr<GraphicsCard>& a, const std::unique_ptr<GraphicsCard>& b)
                {
                    return initInfo.graphicsCardEvaluationHandler(a.get(), b.get());
                });
            }
            else
            {
                std::sort(cards.begin(), cards.end(),[](const std::unique_ptr<GraphicsCard>& a, const std::unique_ptr<GraphicsCard>& b)
                {
                    return a->videoMemory() > b->videoMemory();
                });
            }
            SLAG_CHOSEN_GRAPHICS_CARD = std::move(cards.front());
        }
        return true;
    }

    void cleanup()
    {
        SLAG_CHOSEN_GRAPHICS_CARD = nullptr;
        delete Backend::_current;
        Backend::_current = nullptr;
    }

    GraphicsCard* graphicsCard()
    {
        return SLAG_CHOSEN_GRAPHICS_CARD.get();
    }
}
