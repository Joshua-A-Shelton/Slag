#ifndef SLAG_VULKANUNIFORMSET_H
#define SLAG_VULKANUNIFORMSET_H
#include "../../UniformSet.h"
#include "VulkanUniform.h"
#include "VulkanUniformLayoutInfo.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <spirv_reflect.h>

namespace slag
{
    namespace vulkan
    {

        class VulkanUniformSet: public UniformSet
        {
        public:
            VulkanUniformSet(uint32_t index);
            VulkanUniformSet(const SpvReflectDescriptorSet* description, VkShaderStageFlagBits stage);
            VulkanUniformSet(const VulkanUniformSet&)=delete;
            VulkanUniformSet& operator=(const VulkanUniformSet&)=delete;
            VulkanUniformSet(VulkanUniformSet&& from);
            VulkanUniformSet& operator=(VulkanUniformSet&& from);
            ~VulkanUniformSet()override;
            uint32_t index()override;
            size_t bufferCount()override;
            Uniform* operator[](size_t index)override;
            VkDescriptorSetLayout descriptorSetLayout();
            const std::vector<uint32_t>& dynamicOffsets();
            void merge(VulkanUniformSet&& with);
        private:
            uint32_t _index=0;
            VkShaderStageFlags _accessibleFrom=0;
            VkDescriptorSetLayout _descriptorSetLayout = nullptr;
            std::vector<VulkanUniform> _uniforms;
            std::vector<uint32_t> _dynamicOffsets{};
            VulkanUniformLayoutInfo layoutInfo();
            void setDynamicOffsets();
            void move(VulkanUniformSet&& from);

        };

    } // slag
} // vulkan

#endif //SLAG_VULKANUNIFORMSET_H
