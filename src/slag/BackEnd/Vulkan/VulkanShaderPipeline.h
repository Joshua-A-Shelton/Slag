#ifndef SLAG_VULKANSHADERPIPELINE_H
#define SLAG_VULKANSHADERPIPELINE_H
#include "../../ShaderPipeline.h"
#include "../../Resources/Resource.h"
#include "VulkanDescriptorGroup.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanShaderPipeline: public ShaderPipeline, resources::Resource
        {
        public:
            VulkanShaderPipeline(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately);
            ~VulkanShaderPipeline()override;
            VulkanShaderPipeline(const VulkanShaderPipeline&)=delete;
            VulkanShaderPipeline& operator=(const VulkanShaderPipeline&)=delete;
            VulkanShaderPipeline(VulkanShaderPipeline&& from);
            VulkanShaderPipeline& operator=(VulkanShaderPipeline&& from);
            size_t descriptorGroupCount()override;
            DescriptorGroup* descriptorGroup(size_t index)override;
            DescriptorGroup* operator[](size_t index)override;
            size_t pushConstantRangeCount()override;
            PushConstantRange pushConstantRange(size_t index)override;
            VkPipeline pipeline()const;
            VkPipelineLayout layout()const;
        private:
            void move(VulkanShaderPipeline&& from);
            std::vector<VulkanDescriptorGroup> _descriptorGroups;
            std::vector<PushConstantRange> _pushConstantRanges;
            VkPipeline _pipeline = nullptr;
            VkPipelineLayout _layout = nullptr;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANSHADERPIPELINE_H
