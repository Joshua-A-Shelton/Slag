#include "VulkanExtensions.h"

#include <vulkan/vulkan_core.h>

namespace slag
{
    namespace vulkan
    {
        void VulkanExtensions::init(vkb::Instance& instance)
        {
            slag_vkCmdBindDescriptorBuffersEXT = (PFN_vkCmdBindDescriptorBuffersEXT)instance.fp_vkGetInstanceProcAddr(instance.instance, "vkCmdBindDescriptorBuffersEXT");
            slag_vkGetDescriptorEXT = (PFN_vkGetDescriptorEXT)instance.fp_vkGetInstanceProcAddr(instance.instance, "vkGetDescriptorEXT");
            slag_vkCmdSetDescriptorBufferOffsetsEXT = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)instance.fp_vkGetInstanceProcAddr(instance.instance, "vkCmdSetDescriptorBufferOffsetsEXT");
            slag_vkGetDescriptorSetLayoutSizeEXT = (PFN_vkGetDescriptorSetLayoutSizeEXT)instance.fp_vkGetInstanceProcAddr(instance.instance, "vkGetDescriptorSetLayoutSizeEXT");
            slag_vkGetDescriptorSetLayoutBindingOffsetEXT = (PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)instance.fp_vkGetInstanceProcAddr(instance.instance, "vkGetDescriptorSetLayoutBindingOffsetEXT");
        }
    } // vulkan
} // slag
