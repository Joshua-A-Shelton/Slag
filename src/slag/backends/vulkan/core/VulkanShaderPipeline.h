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
            VulkanShaderPipeline(ShaderCode** shaders, uint32_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription, DescriptorIdentity(*identify)(const DescriptorIdentityParameters&,void*), void* identifyData);
            VulkanShaderPipeline(ShaderCode* computeCode, DescriptorIdentity(*identify)(const DescriptorIdentityParameters&,void*), void* identifyData);
            VulkanShaderPipeline(const VulkanShaderPipeline&)=delete;
            VulkanShaderPipeline& operator=(const VulkanShaderPipeline&)=delete;
            VulkanShaderPipeline(VulkanShaderPipeline&& from);
            VulkanShaderPipeline& operator=(VulkanShaderPipeline&& from);
            virtual ~VulkanShaderPipeline()override;
            virtual PipelineType pipelineType()override;
            virtual uint32_t descriptorGroupCount()override;
            BufferLayout* pushConstants() override;
            virtual VertexDescription* vertexDescription()override;
            virtual DescriptorGroup* descriptorGroup(uint32_t index)override;
            virtual DescriptorGroup* operator[](uint32_t index)override;
            virtual BufferLayout* uniformBufferLayout(uint32_t descriptorGroup,uint32_t descriptorBinding)override;
            virtual BufferLayout* storageBufferLayout(uint32_t descriptorGroup, uint32_t descriptorBinding)override;
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
            std::unique_ptr<VertexDescription> _vertexDescription = nullptr;
            std::vector<VulkanDescriptorGroup> _descriptorGroups;
            std::unique_ptr<BufferLayout> _pushConstants;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,BufferLayout>> _uniformBufferLayouts;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,BufferLayout>> _storageBufferLayouts;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,TexelBufferDescription>> _texelBufferDescriptions;
            uint32_t _xthreads = 0;
            uint32_t _ythreads = 0;
            uint32_t _zthreads = 0;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSHADERPIPELINE_H
