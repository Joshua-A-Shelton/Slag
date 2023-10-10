#ifndef SLAG_VULKANUNIFORMSET_H
#define SLAG_VULKANUNIFORMSET_H
#include "../../UniformSet.h"
#include "VulkanUniform.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <spirv_reflect.h>

namespace slag
{
    namespace vulkan
    {

        class VulkanUniformSet: UniformSet
        {
        public:
            VulkanUniformSet(const SpvReflectDescriptorSet* description, VkShaderStageFlagBits stage);
            ~VulkanUniformSet()override=default;
            uint32_t index()override;
            size_t bufferCount()override;
            Uniform* operator[](size_t index)override;
        private:
            uint32_t _index;
            VkShaderStageFlags _accessibleFrom;
            std::vector<VulkanUniform> _uniforms;
        };

    } // slag
} // vulkan

#endif //SLAG_VULKANUNIFORMSET_H
