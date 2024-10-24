#include "VulkanShader.h"

namespace slag
{
    namespace vulkan
    {
        size_t VulkanShader::descriptorGroupCount()
        {
            return _descriptorGroups.size();
        }

        DescriptorGroup* VulkanShader::descriptorGroup(size_t index)
        {
            return &_descriptorGroups.at(index);
        }

        DescriptorGroup* VulkanShader::operator[](size_t index)
        {
            return &_descriptorGroups[index];
        }


    } // vulkan
} // slag