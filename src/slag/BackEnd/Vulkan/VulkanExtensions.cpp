#include "VulkanExtensions.h"

namespace slag
{
    namespace vulkan
    {
        void VulkanExtensions::initExtensions(VulkanGraphicsCard* card)
        {
            vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(card->device(), "vkCmdBeginRenderingKHR"));
            vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(card->device(), "vkCmdEndRenderingKHR"));
        }
    } // slag
} // vulkan