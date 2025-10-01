#include "VulkanShaderPipeline.h"
#include <slag/spirv/SPIRVReflection.h>

#include "VulkanGraphicsCard.h"
#include "slag/backends/vulkan/VulkanBackend.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {

        struct VulkanShaderModule
        {
        public:
            VkShaderModule shaderModule = nullptr;
            VulkanShaderModule(){}
            VulkanShaderModule(ShaderCode* code)
            {
                VkShaderModuleCreateInfo createVertexInfo = {};
                createVertexInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                createVertexInfo.codeSize = code->dataSize();
                createVertexInfo.pCode = static_cast<const uint32_t*>(code->data());
                if(vkCreateShaderModule(VulkanGraphicsCard::selected()->device(),&createVertexInfo, nullptr,&shaderModule)!= VK_SUCCESS)
                {
                    throw std::runtime_error("invalid shader module");
                }
            }
            VulkanShaderModule(const VulkanShaderModule& other)=delete;
            VulkanShaderModule& operator=(const VulkanShaderModule& other)=delete;
            VulkanShaderModule(VulkanShaderModule&& other)
            {
                std::swap(shaderModule,other.shaderModule);
            }
            VulkanShaderModule& operator=(VulkanShaderModule&& other)
            {
                std::swap(shaderModule,other.shaderModule);
                return *this;
            }
            ~VulkanShaderModule()
            {
                if (shaderModule != nullptr)
                {
                    vkDestroyShaderModule(VulkanGraphicsCard::selected()->device(),shaderModule,nullptr);
                }
            }
        };

        VulkanShaderPipeline::VulkanShaderPipeline(ShaderCode** shaders, uint32_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription, std::string(*rename)(const std::string&,uint32_t,Descriptor::Type, uint32_t,void*), void* renameData)
        {
            _pipelineType = PipelineType::GRAPHICS;
            _xthreads = 0;
            _ythreads = 0;
            _zthreads = 0;
// get the reflection data *********************************************************************************************

            auto reflectionData = spirv::getReflectionData(shaders, shaderCount,rename,renameData);
            _bufferLayouts = std::move(reflectionData.bufferLayouts);
            _texelBufferDescriptions = std::move(reflectionData.texelBufferDescriptions);
            _descriptorGroups.resize(reflectionData.groups.size());
            for (auto i = 0; i < reflectionData.groups.size(); i++)
            {
                auto descriptors = reflectionData.groups[i].descriptors;
                _descriptorGroups[i] = VulkanDescriptorGroup(descriptors.data(),descriptors.size());
            }

// assemble shader stages **********************************************************************************************

            std::vector<VulkanShaderModule> shaderModules(shaderCount);
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaderCount,VkPipelineShaderStageCreateInfo{});

            for (size_t i=0; i < shaderCount; i++)
            {
                shaderModules[i] = VulkanShaderModule(shaders[i]);

                auto& createInfo = shaderStages[i];
                createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                createInfo.stage = VulkanBackend::vulkanizedShaderStage(shaders[i]->stage());
                createInfo.module = shaderModules[i].shaderModule;
                createInfo.pName = "main";
            }


// build the shader ****************************************************************************************************

            VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
            rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationInfo.pNext = nullptr;
            rasterizationInfo.flags = 0;
            rasterizationInfo.depthClampEnable = properties.rasterizationState.depthClampEnable;
            rasterizationInfo.rasterizerDiscardEnable = properties.rasterizationState.rasterizerDicardEnable;
            rasterizationInfo.polygonMode = VulkanBackend::vulkanizedPolygonMode(properties.rasterizationState.drawMode);
            rasterizationInfo.cullMode = VulkanBackend::vulkanizedCullMode(properties.rasterizationState.culling);
            rasterizationInfo.frontFace = VulkanBackend::vulkanizedFrontFace(properties.rasterizationState.frontFacing);
            rasterizationInfo.depthBiasEnable = properties.rasterizationState.depthBiasEnable;
            rasterizationInfo.depthBiasConstantFactor = static_cast<float>(properties.rasterizationState.depthBiasConstantFactor);
            rasterizationInfo.depthBiasClamp = properties.rasterizationState.depthBiasClamp;
            rasterizationInfo.depthBiasSlopeFactor = properties.rasterizationState.depthBiasSlopeFactor;
            rasterizationInfo.lineWidth = properties.rasterizationState.lineThickness;


            VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
            multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleInfo.pNext = nullptr;
            multisampleInfo.flags = 0;
            multisampleInfo.rasterizationSamples = static_cast<VkSampleCountFlagBits>(properties.multiSampleState.rasterizationSamples);
            multisampleInfo.sampleShadingEnable = properties.multiSampleState.sampleShadingEnable;
            multisampleInfo.minSampleShading = properties.multiSampleState.minSampleShading == 0? 0: static_cast<float>(properties.multiSampleState.minSampleShading) / static_cast<float>(properties.multiSampleState.rasterizationSamples);
            //multisampleInfo.pSampleMask = 0;
            multisampleInfo.alphaToCoverageEnable = false;
            multisampleInfo.alphaToOneEnable = properties.multiSampleState.alphaToOneEnable;

            uint32_t colorTargetCount = 0;
            for (auto i=0; i< 8; i++)
            {
                if (framebufferDescription.colorTargets[i]!=Pixels::Format::UNDEFINED)
                {
                    colorTargetCount++;
                }
                else
                {
                    break;
                }
            }
            std::vector<VkPipelineColorBlendAttachmentState> attachmentStates(colorTargetCount,VkPipelineColorBlendAttachmentState{});
            for(size_t i=0; i<attachmentStates.size(); i++)
            {
                auto& colorBlendAttachment = attachmentStates[i];
                auto& colorBlendDescription = properties.blendState.attachmentBlendStates[i];

                colorBlendAttachment.blendEnable = colorBlendDescription.blendingEnabled;
                colorBlendAttachment.srcColorBlendFactor = VulkanBackend::vulkanizedBlendFactor(colorBlendDescription.srcColorBlendFactor);
                colorBlendAttachment.dstColorBlendFactor = VulkanBackend::vulkanizedBlendFactor(colorBlendDescription.dstColorBlendFactor);
                colorBlendAttachment.colorBlendOp = VulkanBackend::vulkanizedBlendOp(colorBlendDescription.colorBlendOperation);
                colorBlendAttachment.srcAlphaBlendFactor = VulkanBackend::vulkanizedBlendFactor(colorBlendDescription.srcAlphaBlendFactor);
                colorBlendAttachment.dstAlphaBlendFactor = VulkanBackend::vulkanizedBlendFactor(colorBlendDescription.dstAlphaBlendFactor);
                colorBlendAttachment.alphaBlendOp = VulkanBackend::vulkanizedBlendOp(colorBlendDescription.alphaBlendOperation);
                colorBlendAttachment.colorWriteMask = VulkanBackend::vulkanizedColorComponentFlags(colorBlendDescription.colorWriteMask);
            }

            VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {};
            colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendingInfo.pNext = nullptr;
            colorBlendingInfo.flags = 0;
            colorBlendingInfo.logicOpEnable = properties.blendState.logicOperationEnable;
            colorBlendingInfo.logicOp = VulkanBackend::vulkanizedLogicOp(properties.blendState.logicalOperation);
            colorBlendingInfo.attachmentCount = attachmentStates.size();
            colorBlendingInfo.pAttachments = attachmentStates.data();

            VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
            depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencilInfo.pNext = nullptr;
            depthStencilInfo.flags = 0;
            depthStencilInfo.depthTestEnable = properties.depthStencilState.depthTestEnable;
            depthStencilInfo.depthWriteEnable = properties.depthStencilState.depthWriteEnable;
            depthStencilInfo.depthCompareOp = VulkanBackend::vulkanizedCompareOp(properties.depthStencilState.depthCompareOperation);
            depthStencilInfo.stencilTestEnable = properties.depthStencilState.stencilTestEnable;
            depthStencilInfo.front.failOp = VulkanBackend::vulkanizedStencilOp(properties.depthStencilState.front.failOp);
            depthStencilInfo.front.passOp = VulkanBackend::vulkanizedStencilOp(properties.depthStencilState.front.passOp);
            depthStencilInfo.front.depthFailOp = VulkanBackend::vulkanizedStencilOp(properties.depthStencilState.front.depthFailOp);
            depthStencilInfo.front.compareOp = VulkanBackend::vulkanizedCompareOp(properties.depthStencilState.front.compareOp);
            depthStencilInfo.front.compareMask = properties.depthStencilState.stencilReadMask;//I think only lest significant digits need to be set
            depthStencilInfo.front.writeMask = properties.depthStencilState.stencilWriteMask;//I think only lest significant digits need to be set
            depthStencilInfo.front.reference = 0;//set via command buffer dynamically
            depthStencilInfo.back.failOp = VulkanBackend::vulkanizedStencilOp(properties.depthStencilState.back.failOp);
            depthStencilInfo.back.passOp = VulkanBackend::vulkanizedStencilOp(properties.depthStencilState.back.passOp);
            depthStencilInfo.back.depthFailOp = VulkanBackend::vulkanizedStencilOp(properties.depthStencilState.back.depthFailOp);
            depthStencilInfo.back.compareOp = VulkanBackend::vulkanizedCompareOp(properties.depthStencilState.back.compareOp);
            depthStencilInfo.back.compareMask = properties.depthStencilState.stencilReadMask;//I think only lest significant digits need to be set
            depthStencilInfo.back.writeMask = properties.depthStencilState.stencilWriteMask;//I think only lest significant digits need to be set
            depthStencilInfo.back.reference = 0;//set via command buffer dynamically
            depthStencilInfo.depthBoundsTestEnable = false;
            depthStencilInfo.minDepthBounds = 0;//we're not doing depth bounds testing, ignore
            depthStencilInfo.maxDepthBounds = 0;//we're not doing depth bounds testing, ignore


            std::vector<VkVertexInputAttributeDescription> attributes;
            std::vector<VkVertexInputBindingDescription> bindingDescriptions;
            uint32_t location = 0;
            attributes.resize(vertexDescription.attributeCount());
            bindingDescriptions.resize(vertexDescription.attributeChannels());
            size_t attIndex = 0;
            for (size_t channel = 0; channel < vertexDescription.attributeChannels(); channel++)
            {
                uint32_t stride = 0;
                for (size_t attribute = 0; attribute < vertexDescription.attributeCount(channel); attribute++)
                {
                    auto& attr = attributes[attIndex];
                    auto& description = vertexDescription.attribute(channel, attribute);
                    attr.location = location;
                    attr.binding = channel;
                    attr.format = VulkanBackend::vulkanizedGraphicsType(description.dataType());
                    if(attr.format == VK_FORMAT_UNDEFINED)
                    {
                        throw std::runtime_error("Unable to convert graphicsType type into underlying API type");
                    }
                    attr.offset = description.offset();
                    location++;
                    attIndex++;
                    size_t end = attr.offset + graphicsTypeSize(description.dataType());
                    if (end > stride)
                    {
                        stride = end;
                    }
                }
                auto& bindingDescription = bindingDescriptions[channel];
                bindingDescription.binding = channel;
                bindingDescription.stride = stride;
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //TODO: I need to make this selectable, but I'm not sure how....
            }

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.pNext = nullptr;
            vertexInputInfo.flags = 0;
            vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
            vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
            vertexInputInfo.vertexAttributeDescriptionCount = attributes.size();
            vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

            VkPipelineInputAssemblyStateCreateInfo assemblyStateInfo = {};
            assemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            assemblyStateInfo.pNext = nullptr;
            assemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            assemblyStateInfo.primitiveRestartEnable = VK_FALSE;

            VkPipelineViewportStateCreateInfo viewportState = {};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.pNext = nullptr;
            viewportState.viewportCount = 1;
            viewportState.pViewports = nullptr;
            viewportState.scissorCount = 1;
            viewportState.pScissors = nullptr;

            std::vector<VkDescriptorSetLayout> layouts(_descriptorGroups.size());

            for(size_t i=0; i < _descriptorGroups.size(); i++)
            {
                layouts[i] = _descriptorGroups[i].layout();
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.pNext = nullptr;
            pipelineLayoutInfo.flags = 0;
            pipelineLayoutInfo.setLayoutCount = layouts.size();
            pipelineLayoutInfo.pSetLayouts = layouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = 0;
            pipelineLayoutInfo.pPushConstantRanges = nullptr;

            if(vkCreatePipelineLayout(static_cast<VkDevice>(VulkanGraphicsCard::selected()->device()),&pipelineLayoutInfo, nullptr,&_pipelineLayout) != VK_SUCCESS)
            {
                throw std::runtime_error("Unable to create shader pipeline layout");
            }

            //TODO: most of the interesting stuff is in here.... I may need to enable more
            std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE,VK_DYNAMIC_STATE_BLEND_CONSTANTS, VK_DYNAMIC_STATE_STENCIL_REFERENCE};
            VkPipelineDynamicStateCreateInfo dynamicInfo{};
            dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicInfo.pDynamicStates = dynamicStates.data();

            VkGraphicsPipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

            pipelineInfo.stageCount = shaderStages.size();
            pipelineInfo.pStages = shaderStages.data();

            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &assemblyStateInfo;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizationInfo;
            pipelineInfo.pMultisampleState = &multisampleInfo;
            pipelineInfo.pColorBlendState = &colorBlendingInfo;
            pipelineInfo.pDepthStencilState = &depthStencilInfo;
            pipelineInfo.layout = _pipelineLayout;
            pipelineInfo.renderPass = VK_NULL_HANDLE;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.pDynamicState = &dynamicInfo;

            auto colorAttachments =std::vector<VkFormat>(colorTargetCount);
            for(int i=0; i< colorTargetCount; i++)
            {
                colorAttachments[i] = VulkanBackend::vulkanizedFormat(framebufferDescription.colorTargets[i]).format;
            }

            VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                    .pNext = nullptr,
                    .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
                    .pColorAttachmentFormats = colorAttachments.data(),
                    .depthAttachmentFormat = VulkanBackend::vulkanizedFormat(framebufferDescription.depthTarget).format,
                    .stencilAttachmentFormat = (bool)(Pixels::aspectFlags(framebufferDescription.depthTarget) & Pixels::AspectFlags::STENCIL) ?  VulkanBackend::vulkanizedFormat(framebufferDescription.depthTarget).format : VK_FORMAT_UNDEFINED
            };

            pipelineInfo.pNext = &pipelineRenderingCreateInfo;

            auto result = vkCreateGraphicsPipelines(VulkanGraphicsCard::selected()->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);

            if (result != VK_SUCCESS)
            {
                auto device = VulkanGraphicsCard::selected()->device();
                vkDestroyPipeline(device,_pipeline,nullptr);
                vkDestroyPipelineLayout(device,_pipelineLayout,nullptr);
                throw std::runtime_error("Unable to create pipeline");
            }

        }

        VulkanShaderPipeline::VulkanShaderPipeline(const ShaderCode& computeCode, std::string(*rename)(const std::string&,uint32_t descriptorGroupIndex,Descriptor::Type type, uint32_t platformBindingIndex,void*), void* renameData)
        {
            _pipelineType = PipelineType::COMPUTE;
            auto computeCodePtr = &const_cast<ShaderCode&>(computeCode);
            auto reflectionData = spirv::getReflectionData(&computeCodePtr, 1,rename,renameData);
            _xthreads = reflectionData.entryPointXDim;
            _ythreads = reflectionData.entryPointYDim;
            _zthreads = reflectionData.entryPointZDim;
            _bufferLayouts = std::move(reflectionData.bufferLayouts);
            _descriptorGroups.resize(reflectionData.groups.size());
            for (auto i = 0; i < reflectionData.groups.size(); i++)
            {
                auto descriptors = reflectionData.groups[i].descriptors;
                _descriptorGroups[i] = VulkanDescriptorGroup(descriptors.data(),descriptors.size());
            }

            VulkanShaderModule shaderModule(computeCodePtr);

            VkPipelineShaderStageCreateInfo computeStageCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_COMPUTE_BIT,.module = shaderModule.shaderModule,.pName = "main"};

            std::vector<VkDescriptorSetLayout> layouts(_descriptorGroups.size());

            for(size_t i=0; i < _descriptorGroups.size(); i++)
            {
                layouts[i] = _descriptorGroups[i].layout();
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.pNext = nullptr;
            pipelineLayoutInfo.pSetLayouts = layouts.data();
            pipelineLayoutInfo.setLayoutCount = layouts.size();

            if(vkCreatePipelineLayout(static_cast<VkDevice>(VulkanGraphicsCard::selected()->device()),&pipelineLayoutInfo, nullptr,&_pipelineLayout) != VK_SUCCESS)
            {
                throw std::runtime_error("Unable to create shader pipeline layout");
            }

            VkComputePipelineCreateInfo computePipelineCreateInfo{};
            computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            computePipelineCreateInfo.pNext = nullptr;
            computePipelineCreateInfo.layout = _pipelineLayout;
            computePipelineCreateInfo.stage = computeStageCreateInfo;
            computePipelineCreateInfo.flags = VK_PIPELINE_CREATE_DISPATCH_BASE;

            auto result = vkCreateComputePipelines(VulkanGraphicsCard::selected()->device(),VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &_pipeline);

            if (result != VK_SUCCESS)
            {
                auto device = VulkanGraphicsCard::selected()->device();
                vkDestroyPipeline(device,_pipeline,nullptr);
                vkDestroyPipelineLayout(device,_pipelineLayout,nullptr);
                throw std::runtime_error("Unable to create pipeline");
            }
        }

        VulkanShaderPipeline::VulkanShaderPipeline(VulkanShaderPipeline&& from)
        {
            move(from);
        }

        VulkanShaderPipeline& VulkanShaderPipeline::operator=(VulkanShaderPipeline&& from)
        {
            move(from);
            return *this;
        }

        VulkanShaderPipeline::~VulkanShaderPipeline()
        {
            auto device = VulkanGraphicsCard::selected()->device();
            vkDestroyPipeline(device,_pipeline,nullptr);
            vkDestroyPipelineLayout(device,_pipelineLayout,nullptr);
        }

        ShaderPipeline::PipelineType VulkanShaderPipeline::pipelineType()
        {
            return _pipelineType;
        }

        uint32_t VulkanShaderPipeline::descriptorGroupCount()
        {
            return _descriptorGroups.size();
        }

        DescriptorGroup* VulkanShaderPipeline::descriptorGroup(uint32_t index)
        {
            return &_descriptorGroups.at(index);
        }

        DescriptorGroup* VulkanShaderPipeline::operator[](uint32_t index)
        {
            return &_descriptorGroups[index];
        }

        BufferLayout* VulkanShaderPipeline::bufferLayout(uint32_t descriptorGroup,uint32_t descriptorBinding)
        {
            auto group = _bufferLayouts.find(descriptorGroup);
            if(group == _bufferLayouts.end())
            {
                return nullptr;
            }
            auto description = group->second.find(descriptorBinding);
            if(description == group->second.end())
            {
                return nullptr;
            }
            return &description->second;
        }

        TexelBufferDescription* VulkanShaderPipeline::texelBufferDescription(uint32_t descriptorGroup,uint32_t descriptorBinding)
        {
            auto group = _texelBufferDescriptions.find(descriptorGroup);
            if(group == _texelBufferDescriptions.end())
            {
                return nullptr;
            }
            auto description = group->second.find(descriptorBinding);
            if(description == group->second.end())
            {
                return nullptr;
            }
            return &description->second;
        }

        uint32_t VulkanShaderPipeline::xComputeThreads()
        {
            return _xthreads;
        }

        uint32_t VulkanShaderPipeline::yComputeThreads()
        {
            return  _ythreads;
        }

        uint32_t VulkanShaderPipeline::zComputeThreads()
        {
            return _zthreads;
        }

        VkPipeline VulkanShaderPipeline::vulkanHandle() const
        {
            return _pipeline;
        }

        VkPipelineLayout VulkanShaderPipeline::vulkanLayout() const
        {
            return _pipelineLayout;
        }

        void VulkanShaderPipeline::move(VulkanShaderPipeline& from)
        {
            _pipelineType = from._pipelineType;
            std::swap(_pipeline,from._pipeline);
            std::swap(_pipelineLayout,from._pipelineLayout);
            _descriptorGroups.swap(from._descriptorGroups);
            _bufferLayouts.swap(from._bufferLayouts);
            _xthreads = from._xthreads;
            _ythreads = from._ythreads;
            _zthreads = from._zthreads;
        }
    } // vulkan
} // slag
