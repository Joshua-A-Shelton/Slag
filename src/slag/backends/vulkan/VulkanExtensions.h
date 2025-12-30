#ifndef SLAG_VULKANEXTENSIONS_H
#define SLAG_VULKANEXTENSIONS_H
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "VkBootstrap.h"
namespace slag
{
    namespace vulkan
    {
        inline PFN_vkCmdBindDescriptorBuffersEXT slag_vkCmdBindDescriptorBuffersEXT = nullptr;
        inline PFN_vkGetDescriptorEXT slag_vkGetDescriptorEXT = nullptr;
        inline PFN_vkCmdSetDescriptorBufferOffsetsEXT slag_vkCmdSetDescriptorBufferOffsetsEXT = nullptr;
        inline PFN_vkGetDescriptorSetLayoutSizeEXT slag_vkGetDescriptorSetLayoutSizeEXT = nullptr;
        inline PFN_vkGetDescriptorSetLayoutBindingOffsetEXT slag_vkGetDescriptorSetLayoutBindingOffsetEXT = nullptr;

        class VulkanExtensions
        {
        public:
            static void init(vkb::Instance& instance);
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANEXTENSIONS_H
