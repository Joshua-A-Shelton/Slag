#ifndef SLAG_VULKANSHADER_H
#define SLAG_VULKANSHADER_H
#include "../../Shader.h"
#include "../../Resources/Resource.h"
#include "VulkanDescriptorGroup.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanShader: public Shader, resources::Resource
        {
        public:
            VulkanShader(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately);
            ~VulkanShader()override;
            VulkanShader(const VulkanShader&)=delete;
            VulkanShader& operator=(const VulkanShader&)=delete;
            VulkanShader(VulkanShader&& from);
            VulkanShader& operator=(VulkanShader&& from);
            size_t descriptorGroupCount()override;
            DescriptorGroup* descriptorGroup(size_t index)override;
            DescriptorGroup* operator[](size_t index)override;
            size_t pushConstantRangeCount()override;
            PushConstantRange pushConstantRange(size_t index)override;
        private:
            void move(VulkanShader&& from);
            std::vector<VulkanDescriptorGroup> _descriptorGroups;
            std::vector<PushConstantRange> _pushConstantRanges;
            VkPipeline _pipeline = nullptr;
            VkPipelineLayout _layout = nullptr;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANSHADER_H
