#include "VulkanPushConstantRange.h"

namespace slag
{
    namespace vulkan
    {
        VulkanPushConstantRange::VulkanPushConstantRange(VkPushConstantRange range)
        {
            _range = range;
        }

        const VkPushConstantRange &VulkanPushConstantRange::range()const
        {
            return _range;
        }
    } // slag
} // vulkan