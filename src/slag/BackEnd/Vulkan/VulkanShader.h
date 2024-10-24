#ifndef SLAG_VULKANSHADER_H
#define SLAG_VULKANSHADER_H
#include "../../Shader.h"
#include "VulkanDescriptorGroup.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanShader: public Shader
        {
        public:
            ~VulkanShader()override;
            size_t descriptorGroupCount()override;
            DescriptorGroup* descriptorGroup(size_t index)override;
            DescriptorGroup* operator[](size_t index)override;
        private:
            std::vector<VulkanDescriptorGroup> _descriptorGroups;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANSHADER_H
