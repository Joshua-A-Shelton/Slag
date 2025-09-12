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
            VulkanShaderPipeline(const ShaderCode& computeCode);
            VulkanShaderPipeline(const VulkanShaderPipeline&)=delete;
            VulkanShaderPipeline& operator=(const VulkanShaderPipeline&)=delete;
            VulkanShaderPipeline(VulkanShaderPipeline&& from);
            VulkanShaderPipeline& operator=(VulkanShaderPipeline&& from);
            virtual ~VulkanShaderPipeline()override;
            virtual PipelineType pipelineType()override;
            virtual uint32_t descriptorGroupCount()override;
            virtual DescriptorGroup* descriptorGroup(size_t index)override;
            virtual DescriptorGroup* operator[](size_t index)override;
            virtual BufferLayout* bufferLayout(uint32_t descriptorGroup,uint32_t descriptorBinding)override;
            virtual TexelBufferDescription* texelBufferDescription(uint32_t descriptorGroup, uint32_t descriptorBinding)override;
            virtual uint32_t xComputeThreads()override;
            virtual uint32_t yComputeThreads()override;
            virtual uint32_t zComputeThreads()override;

            VkPipeline vulkanHandle()const;
            VkPipelineLayout vulkanLayout()const;
        private:
            void move(VulkanShaderPipeline& from);
            PipelineType _pipelineType = PipelineType::GRAPHICS;
            VkPipeline _pipeline = nullptr;
            VkPipelineLayout _pipelineLayout = nullptr;
            std::vector<VulkanDescriptorGroup> _descriptorGroups;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,BufferLayout>> _bufferLayouts;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,TexelBufferDescription>> _texelBufferDescriptions;
            uint32_t _xthreads = 0;
            uint32_t _ythreads = 0;
            uint32_t _zthreads = 0;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSHADERPIPELINE_H
