#ifndef SLAG_VULKANSHADERPIPELINE_H
#define SLAG_VULKANSHADERPIPELINE_H
#include <unordered_map>
#include <slag/Slag.h>
#include <vulkan/vulkan.h>

#include "VulkanDescriptorGroup.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanShaderPipeline: public ShaderPipeline
        {
        public:
            VulkanShaderPipeline(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription);
            VulkanShaderPipeline(const VulkanShaderPipeline&)=delete;
            VulkanShaderPipeline& operator=(const VulkanShaderPipeline&)=delete;
            VulkanShaderPipeline(VulkanShaderPipeline&& from);
            VulkanShaderPipeline& operator=(VulkanShaderPipeline&& from);
            virtual ~VulkanShaderPipeline()override;
            virtual uint32_t descriptorGroupCount()override;
            virtual DescriptorGroup* descriptorGroup(size_t index)override;
            virtual DescriptorGroup* operator[](size_t index)override;
            virtual UniformBufferDescriptorLayout* uniformBufferLayout(uint32_t descriptorGroup,uint32_t descriptorBinding)override;
        private:
            void move(VulkanShaderPipeline& from);
            VkPipeline _pipeline = nullptr;
            VkPipelineLayout _pipelineLayout = nullptr;
            std::vector<VulkanDescriptorGroup> _descriptorGroups;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,UniformBufferDescriptorLayout>> _uniformBufferLayouts;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSHADERPIPELINE_H
