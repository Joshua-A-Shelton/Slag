#include "VulkanEnvironment.h"
#include <slag/Slag.h>
namespace slag
{
    void vulkanDebugHandler(const std::string& message, SlagInitInfo::DebugLevel debugLevel, int32_t messageID)
    {
        std::cout << message << std::endl;
        if (debugLevel != SlagInitInfo::DebugLevel::SLAG_INFO)
        {
            GTEST_FAIL();
        }
    }

    void VulkanEnvironment::SetUp()
    {
#ifndef SLAG_VULKAN_BACKEND
        GTEST_SKIP();
#endif
        Environment::SetUp();
        //clean up previous environment if it exists
        if (slag::slagGraphicsCard()!=nullptr)
        {
            slag::cleanup();
        }
        slag::initialize(SlagInitInfo{.graphicsBackend = VULKAN_GRAPHICS_BACKEND, .slagDebugHandler=vulkanDebugHandler});
    }

    void VulkanEnvironment::TearDown()
    {
        if (slag::slagGraphicsCard()!=nullptr)
        {
            slag::cleanup();
        }
        Environment::TearDown();
    }
} // slag
