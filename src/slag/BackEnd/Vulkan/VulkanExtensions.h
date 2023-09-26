#ifndef SLAG_VULKANEXTENSIONS_H
#define SLAG_VULKANEXTENSIONS_H
#include "VulkanGraphicsCard.h"
namespace slag
{
    namespace vulkan
    {
        class VulkanExtensions
        {
        public:
            inline static PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
            inline static PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;

            static void initExtensions(VulkanGraphicsCard* card);
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANEXTENSIONS_H