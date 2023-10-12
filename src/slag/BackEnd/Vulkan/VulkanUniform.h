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
            VulkanUniform(SpvReflectDescriptorBinding* binding, VkShaderStageFlagBits shaderStage);
            VulkanUniform(const VulkanUniform&)=delete;
            VulkanUniform& operator=(const VulkanUniform&)=delete;
            VulkanUniform(VulkanUniform&& from);
            VulkanUniform& operator=(VulkanUniform&& from);
            ~VulkanUniform()override=default;
            UniformType uniformType()override;
            size_t descriptorCount()override;
            const std::string& name()override;
            uint32_t binding()override;
            void merge(VulkanUniform&& with);
            VkDescriptorSetLayoutBinding vulkanBindingInfo();
            VkDescriptorType vulkanDescriptorType();
            static bool compareBinding(Uniform& uniform1, Uniform& uniform2);
        private:
            uint32_t _binding=0;
            uint32_t _bufferSize=0;
            VkShaderStageFlagBits _accessibleFrom;
            std::string _name;
            VkDescriptorType _descriptorType;
            std::vector<UniformDescriptor> _descriptors;
            void move(VulkanUniform&& from);
        };

    } // slag
} // vulkan

#endif //SLAG_VULKANUNIFORM_H
