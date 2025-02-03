#ifndef SLAG_VULKANSHADERPIPELINE_H
#define SLAG_VULKANSHADERPIPELINE_H
#include "../../ShaderPipeline.h"
#include "../../Resources/Resource.h"
#include "VulkanDescriptorGroup.h"
#include <spirv_reflect.h>
#include "VulkanLib.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanShaderPipeline: public ShaderPipeline, resources::Resource
        {
        public:
            VulkanShaderPipeline(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately);
            VulkanShaderPipeline(ShaderModule** modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately);
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

            struct VulkanShaderData
            {
            public:
                VkShaderModule shaderModule= nullptr;
                ShaderStages stageFlags;
                SpvReflectShaderModule reflectModule;
                VulkanShaderData(ShaderModule& module): stageFlags(module.stage())
                {
                    VkShaderModuleCreateInfo createVertexInfo = {};
                    createVertexInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                    createVertexInfo.codeSize = module.dataSize();
                    createVertexInfo.pCode = reinterpret_cast<const uint32_t*>(module.data());
                    if(vkCreateShaderModule(VulkanLib::card()->device(),&createVertexInfo, nullptr,&shaderModule)!= VK_SUCCESS)
                    {
                        throw std::runtime_error("invalid shader module");
                    }
                    spvReflectCreateShaderModule(module.dataSize(),module.data(),&reflectModule);
                }
                ~VulkanShaderData()
                {
                    if(shaderModule)
                    {
                        vkDestroyShaderModule(VulkanLib::card()->device(), shaderModule, nullptr);
                        spvReflectDestroyShaderModule(&reflectModule);
                    }
                }
                VulkanShaderData(VulkanShaderData&& from):stageFlags(from.stageFlags)
                {
                    std::swap(shaderModule,from.shaderModule);
                    std::swap(reflectModule,from.reflectModule);
                }
                VulkanShaderData& operator=(VulkanShaderData&& from)
                {
                    std::swap(shaderModule,from.shaderModule);
                    std::swap(reflectModule,from.reflectModule);
                    std::swap(stageFlags,from.stageFlags);
                    return *this;
                }
            };
            void constructPipeline(DescriptorGroup* const* descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties,
                                   VertexDescription* vertexDescription,
                                   const FrameBufferDescription& frameBufferDescription, const std::vector<VulkanShaderData>& shaderStageData,
                                   std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
                                   size_t vertexStageIndex);
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANSHADERPIPELINE_H
