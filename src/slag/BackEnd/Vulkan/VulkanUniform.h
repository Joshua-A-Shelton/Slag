#ifndef SLAG_VULKANUNIFORM_H
#define SLAG_VULKANUNIFORM_H
#include "../../Uniform.h"
#include "../../UniformDescriptor.h"
#include <spirv_reflect.h>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {

        class VulkanUniform: public Uniform
        {
        public:
            VulkanUniform(SpvReflectDescriptorBinding* binding);
            ~VulkanUniform()override=default;
            UniformType uniformType()override;
            size_t descriptorCount()override;
            const std::string& name()override;
            uint32_t binding()override;
        private:
            uint32_t _binding;
            std::string _name;
            VkDescriptorType _descriptorType;
            std::vector<UniformDescriptor> _descriptors;
        };

    } // slag
} // vulkan

#endif //SLAG_VULKANUNIFORM_H
