#ifndef SLAG_VULKANPUSHCONSTANTRANGE_H
#define SLAG_VULKANPUSHCONSTANTRANGE_H

#include <vulkan/vulkan.h>
#include "../../PushConstantRange.h"
namespace slag
{
    namespace vulkan
    {

        class VulkanPushConstantRange: public PushConstantRange
        {
        public:
            VulkanPushConstantRange(VkPushConstantRange range);
            const VkPushConstantRange& range()const;
        private:
            VkPushConstantRange _range;
        };

    } // slag
} // vulkan

#endif //SLAG_VULKANPUSHCONSTANTRANGE_H
