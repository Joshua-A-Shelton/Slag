#include "VulkanComputeShader.h"
#include "VulkanLib.h"
#include <spirv_reflect.h>

namespace slag
{
    namespace vulkan
    {
        VulkanComputeShader::VulkanComputeShader(const std::vector<char>& code)
        {
            VkDescriptorSetLayout descriptorSetLayout;
            SpvReflectShaderModule reflectedModule;
            spvReflectCreateShaderModule(code.size(),code.data(),&reflectedModule);

            uint32_t groupCount = 0;
            auto result = spvReflectEnumerateDescriptorSets(&reflectedModule,&groupCount, nullptr);
            assert(result == SPV_REFLECT_RESULT_SUCCESS && "Unable to get reflection data");

            for(uint32_t i =0; i< groupCount; i++)
            {
                auto binding = reflectedModule.descriptor_bindings[i];
                auto set = spvReflectGetDescriptorSet(&reflectedModule,binding.set,&result);
                if(set!= nullptr)
                {
                    _uniformSets.push_back(VulkanUniformSet(set, VK_SHADER_STAGE_COMPUTE_BIT));
                }
            }



            std::vector<VkDescriptorSetLayout> setLayouts;
            for(auto& set: _uniformSets)
            {
                setLayouts.push_back(set.descriptorSetLayout());
            }

            //push constants
            uint32_t blockCount = 0;
            result = spvReflectEnumeratePushConstantBlocks(&reflectedModule,&blockCount, nullptr);
            assert(result == SPV_REFLECT_RESULT_SUCCESS && "Unable to get push constant data");
            for(uint32_t i=0; i< blockCount; i++)
            {
                auto& pushConstantRangeData = *spvReflectGetPushConstantBlock(&reflectedModule,i,&result);
                VkPushConstantRange range{};
                range.size = pushConstantRangeData.size;
                range.offset = pushConstantRangeData.offset;
                range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                _pushConstantRanges.push_back(VulkanPushConstantRange(range));
            }

            spvReflectDestroyShaderModule(&reflectedModule);

            std::vector<VkPushConstantRange> setPushConstants;
            for(auto& range: _pushConstantRanges)
            {
                setPushConstants.push_back(range.range());
            }


            VkShaderModuleCreateInfo createComputeInfo = {};
            createComputeInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createComputeInfo.codeSize = code.size();
            createComputeInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule cshaderModule = nullptr;
            if(vkCreateShaderModule(VulkanLib::graphicsCard()->device(),&createComputeInfo, nullptr,&cshaderModule)!= VK_SUCCESS)
            {
                throw std::runtime_error("Unable to use compute shader");
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.pNext = nullptr;
            pipelineLayoutInfo.flags = 0;
            pipelineLayoutInfo.setLayoutCount = setLayouts.size();
            pipelineLayoutInfo.pSetLayouts = setLayouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = setPushConstants.size();
            pipelineLayoutInfo.pPushConstantRanges = setPushConstants.data();


            if(vkCreatePipelineLayout(static_cast<VkDevice>(VulkanLib::graphicsCard()->device()),&pipelineLayoutInfo, nullptr,&_pipelineLayout)!= VK_SUCCESS)
            {
                throw std::runtime_error("Unable to create shader layout");
            }


            VkPipelineShaderStageCreateInfo stageinfo{};
            stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageinfo.pNext = nullptr;
            stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            stageinfo.module = cshaderModule;
            stageinfo.pName = "main";

            VkComputePipelineCreateInfo computePipelineCreateInfo{};
            computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            computePipelineCreateInfo.pNext = nullptr;
            computePipelineCreateInfo.layout = _pipelineLayout;
            computePipelineCreateInfo.stage = stageinfo;

            if(vkCreateComputePipelines(VulkanLib::graphicsCard()->device(),VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &_pipeline)!= VK_SUCCESS)
            {
                vkDestroyShaderModule(VulkanLib::graphicsCard()->device(),cshaderModule, nullptr);
                vkDestroyPipeline(VulkanLib::graphicsCard()->device(),static_cast<VkPipeline>(_pipeline), nullptr);
                vkDestroyPipelineLayout(VulkanLib::graphicsCard()->device(),static_cast<VkPipelineLayout>(_pipelineLayout),nullptr);
                throw std::runtime_error("Unable to create compute shader");
            }


            vkDestroyShaderModule(VulkanLib::graphicsCard()->device(),cshaderModule, nullptr);

            VkPipeline pipeline = _pipeline;
            VkPipelineLayout pipelineLayout = _pipelineLayout;

            freeResources = [=]()
            {
                vkDestroyPipeline(VulkanLib::graphicsCard()->device(),static_cast<VkPipeline>(pipeline), nullptr);
                vkDestroyPipelineLayout(VulkanLib::graphicsCard()->device(),static_cast<VkPipelineLayout>(pipelineLayout),nullptr);
            };
        }

        VulkanComputeShader::~VulkanComputeShader()
        {
            if(_pipeline)
            {
                smartDestroy();
            }
        }

        UniformSet* VulkanComputeShader::getUniformSet(size_t index)
        {
            return &_uniformSets[index];
        }

        size_t VulkanComputeShader::uniformSetCount()
        {
            return _uniformSets.size();
        }

        PushConstantRange* VulkanComputeShader::getPushConstantRange(size_t index)
        {
            return &_pushConstantRanges[index];
        }

        size_t VulkanComputeShader::pushConstantRangeCount()
        {
            return _pushConstantRanges.size();
        }

        void* VulkanComputeShader::GPUID()
        {
            return _pipeline;
        }

        VkPipelineLayout VulkanComputeShader::layout()
        {
            return _pipelineLayout;
        }

        VkPipeline VulkanComputeShader::pipeline()
        {
            return _pipeline;
        }
    } // vulkan
} // slag