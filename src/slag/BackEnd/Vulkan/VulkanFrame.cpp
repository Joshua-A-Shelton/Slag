#include "VulkanFrame.h"

namespace slag
{
    namespace vulkan
    {
        VulkanFrame::VulkanFrame(VkImage backBuffer, VkImageView view, VulkanSwapchain* from)
        {
            _from = from;
            //_backBuffer = new VulkanTexture(backBuffer, false, view, true,)
        }
    } // vulkan
} // slag