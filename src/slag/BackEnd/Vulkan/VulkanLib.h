#ifndef SLAG_VULKANLIB_H
#define SLAG_VULKANLIB_H
#include <vulkan/vulkan.h>
#include "VulkanGraphicsCard.h"
namespace slag
{
    namespace vulkan
    {
        class VulkanLib
        {
        public:
            static bool initialize();
            static void cleanup();

            static VkInstance instance();
            static VulkanGraphicsCard* graphicsCard();
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANLIB_H