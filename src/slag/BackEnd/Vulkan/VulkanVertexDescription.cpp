#include <cassert>
#include "VulkanVertexDescription.h"
#include "VulkanTexture.h"

namespace slag
{
    namespace vulkan
    {
        VulkanVertexDescription::VulkanVertexDescription(VertexDescription &description)
        {
            uint32_t offset = 0;
            for(uint32_t i=0; i< description.attributeCount(); i++)
            {
                VkFormat format = formatFromAttribute(description[i].format,description[i].count);
                _attributeDescriptions.push_back({i,0,format,offset});
                offset+=VulkanTexture::formatSize(format);
            }
            _bindingDescription = {0,offset,VK_VERTEX_INPUT_RATE_VERTEX};
        }

        VkFormat VulkanVertexDescription::formatFromAttribute(VertexAttributes::VertexAttributeType attributeType, uint8_t count)
        {
            assert(count > 0 && count < 5 && "count must be between 1 and 4 inclusive");
            switch (attributeType)
            {
                case VertexAttributes::INT_8:
                    switch (count)
                    {
                        case 1:
                            return VkFormat::VK_FORMAT_R8_SINT;
                        case 2:
                            return VkFormat::VK_FORMAT_R8G8_SINT;
                        case 3:
                            return VkFormat::VK_FORMAT_R8G8B8_SINT;
                        case 4:
                            return VkFormat::VK_FORMAT_R8G8B8A8_SINT;
                    }
                    break;
                case VertexAttributes::UINT_8:
                    switch (count)
                    {
                        case 1:
                            return VkFormat::VK_FORMAT_R8_UINT;
                        case 2:
                            return VkFormat::VK_FORMAT_R8G8_UINT;
                        case 3:
                            return VkFormat::VK_FORMAT_R8G8B8_UINT;
                        case 4:
                            return VkFormat::VK_FORMAT_R8G8B8A8_UINT;
                    }
                    break;
                case VertexAttributes::INT_16:
                    switch (count)
                    {
                        case 1:
                            return VkFormat::VK_FORMAT_R16_SINT;
                        case 2:
                            return VkFormat::VK_FORMAT_R16G16_SINT;
                        case 3:
                            return VkFormat::VK_FORMAT_R16G16B16_SINT;
                        case 4:
                            return VkFormat::VK_FORMAT_R16G16B16A16_SINT;
                    }
                    break;
                case VertexAttributes::UINT_16:
                    switch (count)
                    {
                        case 1:
                            return VkFormat::VK_FORMAT_R16_UINT;
                        case 2:
                            return VkFormat::VK_FORMAT_R16G16_UINT;
                        case 3:
                            return VkFormat::VK_FORMAT_R16G16B16_UINT;
                        case 4:
                            return VkFormat::VK_FORMAT_R16G16B16A16_UINT;
                    }
                    break;
                case VertexAttributes::INT_32:
                    switch (count)
                    {
                        case 1:
                            return VkFormat::VK_FORMAT_R32_SINT;
                        case 2:
                            return VkFormat::VK_FORMAT_R32G32_SINT;
                        case 3:
                            return VkFormat::VK_FORMAT_R32G32B32_SINT;
                        case 4:
                            return VkFormat::VK_FORMAT_R32G32B32A32_SINT;
                    }
                    break;
                case VertexAttributes::UINT_32:
                    switch (count)
                    {
                        case 1:
                            return VkFormat::VK_FORMAT_R32_UINT;
                        case 2:
                            return VkFormat::VK_FORMAT_R32G32_UINT;
                        case 3:
                            return VkFormat::VK_FORMAT_R32G32B32_UINT;
                        case 4:
                            return VkFormat::VK_FORMAT_R32G32B32A32_UINT;
                    }
                    break;
                case VertexAttributes::INT_64:
                    switch (count)
                    {
                        case 1:
                            return VkFormat::VK_FORMAT_R64_SINT;
                        case 2:
                            return VkFormat::VK_FORMAT_R64G64_SINT;
                        case 3:
                            return VkFormat::VK_FORMAT_R64G64B64_SINT;
                        case 4:
                            return VkFormat::VK_FORMAT_R64G64B64A64_SINT;
                    }
                    break;
                case VertexAttributes::UINT_64:
                    switch (count)
                    {
                        case 1:
                            return VkFormat::VK_FORMAT_R64_UINT;
                        case 2:
                            return VkFormat::VK_FORMAT_R64G64_UINT;
                        case 3:
                            return VkFormat::VK_FORMAT_R64G64B64_UINT;
                        case 4:
                            return VkFormat::VK_FORMAT_R64G64B64A64_UINT;
                    }
                    break;
                case VertexAttributes::FLOAT:
                    switch (count)
                    {
                        case 1:
                            return VkFormat::VK_FORMAT_R32_SFLOAT;
                        case 2:
                            return VkFormat::VK_FORMAT_R32G32_SFLOAT;
                        case 3:
                            return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
                        case 4:
                            return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
                    }
                    break;
                case VertexAttributes::DOUBLE:
                    switch (count)
                    {
                        case 1:
                            return VkFormat::VK_FORMAT_R64_SFLOAT;
                        case 2:
                            return VkFormat::VK_FORMAT_R64G64_SFLOAT;
                        case 3:
                            return VkFormat::VK_FORMAT_R64G64B64_SFLOAT;
                        case 4:
                            return VkFormat::VK_FORMAT_R64G64B64A64_SFLOAT;
                    }
                    break;
            }
            return VkFormat::VK_FORMAT_UNDEFINED;
        }

        std::vector<VkVertexInputAttributeDescription> &VulkanVertexDescription::attributeDescriptions()
        {
            return _attributeDescriptions;
        }

        VkVertexInputBindingDescription &VulkanVertexDescription::bindingDescription()
        {
            return _bindingDescription;
        }
    } // slag
} // vulkan