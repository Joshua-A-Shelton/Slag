#ifndef SLAG_VULKANVERTEXDESCRIPTION_H
#define SLAG_VULKANVERTEXDESCRIPTION_H
#include "../../VertexDescription.h"
#include "vulkan/vulkan.h"
namespace slag
{
    namespace vulkan
    {

        class VulkanVertexDescription
        {
        public:
            VulkanVertexDescription()=default;
            VulkanVertexDescription(VertexDescription& description);
            std::vector<VkVertexInputAttributeDescription>& attributeDescriptions();
            VkVertexInputBindingDescription& bindingDescription();
            static VkFormat formatFromAttribute(VertexAttributes::VertexAttributeType attributeType, uint8_t count);
        private:
            VkVertexInputBindingDescription _bindingDescription;
            std::vector<VkVertexInputAttributeDescription> _attributeDescriptions;
        };

    } // slag
} // vulkan

#endif //SLAG_VULKANVERTEXDESCRIPTION_H
