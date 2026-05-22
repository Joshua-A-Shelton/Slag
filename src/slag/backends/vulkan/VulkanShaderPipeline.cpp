#include "VulkanShaderPipeline.h"

#include "VulkanBackend.h"
#include "slag/exceptions/InvalidShaderVertexBindingError.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"
#include <spirv_reflect.h>

namespace slag
{
    namespace vulkan
    {
        VulkanShaderPipeline::VulkanShaderPipeline(
            VulkanGraphicsCard* graphicsCard,
            const VertexDescription& vertexDescription,
            const ShaderCode& vertexShader,
            const ShaderCode& fragmentShader,
            const PipelineState& pipelineState,
            const FramebufferDescription& framebufferDescription)
        {
            SLAG_ASSERT(vertexShader.language == ShaderLanguage::SPIRV && "Shader code must be SPIRV for Vulkan");
            SLAG_ASSERT(fragmentShader.language == ShaderLanguage::SPIRV && "Shader code must be SPIRV for Vulkan");
            _graphicsCard = graphicsCard;


            VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
            initNativeRasterizationInfo(pipelineState.rasterizationState,&rasterizationCreateInfo);

            VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
            initMultisampleInfo(pipelineState.multiSampleState,&multisampleCreateInfo);

            VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
            auto blendData = initColorAttachmentInfo(pipelineState.blendState,framebufferDescription,&colorBlendCreateInfo);

            VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
            initDepthAttachmentInfo(pipelineState.depthStencilState,&depthStencilInfo);

            std::vector<VkVertexInputAttributeDescription> attributes;
            std::vector<VkVertexInputBindingDescription> bindingDescriptions;
            struct AttributeKey
            {
                uint32_t location;
                SpvReflectFormat format;
            };
            std::unordered_map<std::string,AttributeKey> attributeMap;

            SpvReflectShaderModule vertexReflectionModule;

            auto result = spvReflectCreateShaderModule(vertexShader.codeLength,vertexShader.code,&vertexReflectionModule);
            if (result != SPV_REFLECT_RESULT_SUCCESS)
            {
                spvReflectDestroyShaderModule(&vertexReflectionModule);
                throw InvalidShaderVertexBindingError("Unable to read shader code");
            }
            for (auto i=0u; i< vertexReflectionModule.input_variable_count; ++i)
            {
                auto inputVariable = vertexReflectionModule.input_variables[i];
                auto attInput = attributeMap.find(inputVariable->semantic);
                if (attInput == attributeMap.end())
                {
                    spvReflectDestroyShaderModule(&vertexReflectionModule);
                    throw InvalidShaderVertexBindingError("Vertex shader has duplicate attribute name");
                }
                attributeMap[inputVariable->semantic] = {inputVariable->location, inputVariable->format};
            }
            spvReflectDestroyShaderModule(&vertexReflectionModule);


            for (auto i=0u; i< vertexDescription.bindingCount(); ++i)
            {
                auto& binding = vertexDescription[i];
                bindingDescriptions.emplace_back(binding.bindingIndex(),binding.stride(),VK_VERTEX_INPUT_RATE_VERTEX);
                for (auto j=0u; j< binding.attributeCount(); ++j)
                {
                    auto& attribute = binding[j];

                    auto attInput = attributeMap.find(attribute.name());
                    if (attInput == attributeMap.end())
                    {
                        throw InvalidShaderVertexBindingError("Attribute '" + attribute.name() + "' is not present in shader");
                    }
                    //TODO: see if the type of the attribute matches

                    attributes.emplace_back(attInput->second.location,binding.bindingIndex(),VulkanBackend::nativeFormat(attribute.loadAs()).format,attribute.offset());
                }
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


            VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE,VK_DYNAMIC_STATE_BLEND_CONSTANTS, VK_DYNAMIC_STATE_STENCIL_REFERENCE};
            VkPipelineDynamicStateCreateInfo dynamicInfo{};
            dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicInfo.dynamicStateCount = sizeof(dynamicStates)/sizeof(VkDynamicState);
            dynamicInfo.pDynamicStates = dynamicStates;


            VkGraphicsPipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

            VkShaderDescriptorSetAndBindingMappingInfoEXT mappings{.sType = VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT};
            VkDescriptorSetAndBindingMappingEXT bindingMapping{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT};
            bindingMapping.descriptorSet = 0;
            bindingMapping.firstBinding = 0;
            bindingMapping.bindingCount = 1;
            bindingMapping.resourceMask = VK_SPIRV_RESOURCE_TYPE_ALL_EXT;
            bindingMapping.source = VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_DATA_EXT;
            bindingMapping.sourceData.pushDataOffset = 0;
            mappings.mappingCount = 1;
            mappings.pMappings = &bindingMapping;

            VkShaderModuleCreateInfo shaderModuleInfo[2];
            shaderModuleInfo[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderModuleInfo[0].pNext = &mappings;
            shaderModuleInfo[0].flags = 0;
            shaderModuleInfo[0].codeSize = vertexShader.codeLength;
            shaderModuleInfo[0].pCode = static_cast<uint32_t*>(vertexShader.code);

            shaderModuleInfo[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderModuleInfo[1].pNext = &mappings;
            shaderModuleInfo[1].flags = 0;
            shaderModuleInfo[1].codeSize = fragmentShader.codeLength;
            shaderModuleInfo[1].pCode = static_cast<uint32_t*>(fragmentShader.code);

            VkPipelineShaderStageCreateInfo shaderStages[2];
            shaderStages[0] =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = &shaderModuleInfo[0],
                .flags = 0,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = nullptr,
                .pName = "main",
                .pSpecializationInfo = nullptr
            };
            shaderStages[1] =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = &shaderModuleInfo[1],
                .flags = 0,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = nullptr,
                .pName = "main",
                .pSpecializationInfo = nullptr
            };
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages;

            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &assemblyStateInfo;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizationCreateInfo;
            pipelineInfo.pMultisampleState = &multisampleCreateInfo;
            pipelineInfo.pColorBlendState = &colorBlendCreateInfo;
            pipelineInfo.pDepthStencilState = &depthStencilInfo;
            pipelineInfo.layout = nullptr;
            pipelineInfo.renderPass = VK_NULL_HANDLE;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.pDynamicState = &dynamicInfo;

            auto colorAttachments =std::vector<VkFormat>(blendData.size());
            for(int i=0; i< blendData.size(); i++)
            {
                colorAttachments[i] = VulkanBackend::nativeFormat(framebufferDescription.colorFormats[i]).format;
            }

            VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                    .pNext = nullptr,
                    .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
                    .pColorAttachmentFormats = colorAttachments.data(),
                    .depthAttachmentFormat = VulkanBackend::nativeFormat(framebufferDescription.depthFormat).format,
                    .stencilAttachmentFormat = (bool)(Pixel::aspectFlags(framebufferDescription.depthFormat) & PixelAspectFlags::STENCIL_FLAG) ?  VulkanBackend::nativeFormat(framebufferDescription.depthFormat).format : VK_FORMAT_UNDEFINED
            };

            pipelineInfo.pNext = &pipelineRenderingCreateInfo;

            VkPipelineCreateFlags2CreateInfo createFlags{.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,.pNext = nullptr,.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT};
            pipelineRenderingCreateInfo.pNext = &createFlags;

            if (vkCreateGraphicsPipelines(_graphicsCard->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline)!=VK_SUCCESS)
            {
                throw ResourceCreationError("Unable to create shader pipeline");
            }

        }

        VulkanShaderPipeline::~VulkanShaderPipeline()
        {
            if (_pipeline!=nullptr)
            {
                vkDestroyPipeline(_graphicsCard->device(), _pipeline, nullptr);
            }
        }

        VulkanShaderPipeline::VulkanShaderPipeline(VulkanShaderPipeline&& from) noexcept
        {
            move(from);
        }

        VulkanShaderPipeline& VulkanShaderPipeline::operator=(VulkanShaderPipeline&& from) noexcept
        {
            move(from);
            return *this;
        }

        ShaderPipelineType VulkanShaderPipeline::type()
        {
            return _type;
        }

        GraphicsCard* VulkanShaderPipeline::graphicsCard()
        {
            return _graphicsCard;
        }

        VkPipeline VulkanShaderPipeline::vulkanHandle() const
        {
            return _pipeline;
        }

        void VulkanShaderPipeline::initNativeRasterizationInfo(
            const RasterizationState& slagRasterizationState,
            VkPipelineRasterizationStateCreateInfo* outRasterizationStateInfo)
        {
            outRasterizationStateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            outRasterizationStateInfo->pNext = nullptr;
            outRasterizationStateInfo->flags = 0;
            outRasterizationStateInfo->depthClampEnable = slagRasterizationState.depthClampEnable;
            outRasterizationStateInfo->rasterizerDiscardEnable = slagRasterizationState.rasterizerDicardEnable;
            outRasterizationStateInfo->polygonMode = VulkanBackend::nativePolygonMode(slagRasterizationState.drawMode);
            outRasterizationStateInfo->cullMode = VulkanBackend::nativeCullMode(slagRasterizationState.culling);
            outRasterizationStateInfo->frontFace = VulkanBackend::nativeFrontFace(slagRasterizationState.frontFacing);
            outRasterizationStateInfo->depthBiasEnable = slagRasterizationState.depthBiasEnable;
            outRasterizationStateInfo->depthBiasConstantFactor = static_cast<float>(slagRasterizationState.depthBiasConstantFactor);
            outRasterizationStateInfo->depthBiasClamp = slagRasterizationState.depthBiasClamp;
            outRasterizationStateInfo->depthBiasSlopeFactor = slagRasterizationState.depthBiasSlopeFactor;
            outRasterizationStateInfo->lineWidth = slagRasterizationState.lineThickness;
        }

        void VulkanShaderPipeline::initMultisampleInfo(const MultiSampleState& multiSampleState,
            VkPipelineMultisampleStateCreateInfo* outMultisampleStateInfo)
        {
            outMultisampleStateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            outMultisampleStateInfo->pNext = nullptr;
            outMultisampleStateInfo->flags = 0;
            outMultisampleStateInfo->rasterizationSamples = static_cast<VkSampleCountFlagBits>(multiSampleState.rasterizationSamples);
            outMultisampleStateInfo->sampleShadingEnable = multiSampleState.sampleShadingEnable;
            outMultisampleStateInfo->minSampleShading = multiSampleState.minSampleShading == 0? 0: static_cast<float>(multiSampleState.minSampleShading) / static_cast<float>(multiSampleState.rasterizationSamples);
            //outMultisampleStateInfo->pSampleMask = 0;
            outMultisampleStateInfo->alphaToCoverageEnable = false;
            outMultisampleStateInfo->alphaToOneEnable = multiSampleState.alphaToOneEnable;
        }

        std::vector<VkPipelineColorBlendAttachmentState> VulkanShaderPipeline::initColorAttachmentInfo(
            const BlendState& slagBlendState,
            const FramebufferDescription& frameBufferDescription,
            VkPipelineColorBlendStateCreateInfo* outPipelineColorBlendStateInfo)
        {

            uint32_t colorTargetCount = 0;
            for (auto i=0; i< 8; i++)
            {
                if (frameBufferDescription.colorFormats[i]!=PixelFormat::UNDEFINED)
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
                auto& colorBlendDescription = slagBlendState.attachmentBlendStates[i];

                colorBlendAttachment.blendEnable = colorBlendDescription.blendingEnabled;
                colorBlendAttachment.srcColorBlendFactor = VulkanBackend::nativeBlendFactor(colorBlendDescription.srcColorBlendFactor);
                colorBlendAttachment.dstColorBlendFactor = VulkanBackend::nativeBlendFactor(colorBlendDescription.dstColorBlendFactor);
                colorBlendAttachment.colorBlendOp = VulkanBackend::nativeBlendOp(colorBlendDescription.colorBlendOperation);
                colorBlendAttachment.srcAlphaBlendFactor = VulkanBackend::nativeBlendFactor(colorBlendDescription.srcAlphaBlendFactor);
                colorBlendAttachment.dstAlphaBlendFactor = VulkanBackend::nativeBlendFactor(colorBlendDescription.dstAlphaBlendFactor);
                colorBlendAttachment.alphaBlendOp = VulkanBackend::nativeBlendOp(colorBlendDescription.alphaBlendOperation);
                colorBlendAttachment.colorWriteMask = VulkanBackend::nativeColorComponentFlags(colorBlendDescription.colorWriteMask);
            }

            outPipelineColorBlendStateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            outPipelineColorBlendStateInfo->pNext = nullptr;
            outPipelineColorBlendStateInfo->flags = 0;
            outPipelineColorBlendStateInfo->logicOpEnable = slagBlendState.logicOperationEnable;
            outPipelineColorBlendStateInfo->logicOp = VulkanBackend::nativeLogicOp(slagBlendState.logicalOperation);
            outPipelineColorBlendStateInfo->attachmentCount = attachmentStates.size();
            outPipelineColorBlendStateInfo->pAttachments = attachmentStates.data();

            return attachmentStates;
        }

        void VulkanShaderPipeline::initDepthAttachmentInfo(const DepthStencilState& slagDepthStencilState,
            VkPipelineDepthStencilStateCreateInfo* outDepthStencilStateInfo)
        {
            outDepthStencilStateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            outDepthStencilStateInfo->pNext = nullptr;
            outDepthStencilStateInfo->flags = 0;
            outDepthStencilStateInfo->depthTestEnable = slagDepthStencilState.depthTestEnable;
            outDepthStencilStateInfo->depthWriteEnable = slagDepthStencilState.depthWriteEnable;
            outDepthStencilStateInfo->depthCompareOp = VulkanBackend::nativeCompareOp(slagDepthStencilState.depthCompareOperation);
            outDepthStencilStateInfo->stencilTestEnable = slagDepthStencilState.stencilTestEnable;
            outDepthStencilStateInfo->front.failOp = VulkanBackend::nativeStencilOp(slagDepthStencilState.front.failOp);
            outDepthStencilStateInfo->front.passOp = VulkanBackend::nativeStencilOp(slagDepthStencilState.front.passOp);
            outDepthStencilStateInfo->front.depthFailOp = VulkanBackend::nativeStencilOp(slagDepthStencilState.front.depthFailOp);
            outDepthStencilStateInfo->front.compareOp = VulkanBackend::nativeCompareOp(slagDepthStencilState.front.compareOp);
            outDepthStencilStateInfo->front.compareMask = slagDepthStencilState.stencilReadMask;//I think only least significant digits need to be set
            outDepthStencilStateInfo->front.writeMask = slagDepthStencilState.stencilWriteMask;//I think only lest significant digits need to be set
            outDepthStencilStateInfo->front.reference = 0;//set via command buffer dynamically
            outDepthStencilStateInfo->back.failOp = VulkanBackend::nativeStencilOp(slagDepthStencilState.back.failOp);
            outDepthStencilStateInfo->back.passOp = VulkanBackend::nativeStencilOp(slagDepthStencilState.back.passOp);
            outDepthStencilStateInfo->back.depthFailOp = VulkanBackend::nativeStencilOp(slagDepthStencilState.back.depthFailOp);
            outDepthStencilStateInfo->back.compareOp = VulkanBackend::nativeCompareOp(slagDepthStencilState.back.compareOp);
            outDepthStencilStateInfo->back.compareMask = slagDepthStencilState.stencilReadMask;//I think only lest significant digits need to be set
            outDepthStencilStateInfo->back.writeMask = slagDepthStencilState.stencilWriteMask;//I think only lest significant digits need to be set
            outDepthStencilStateInfo->back.reference = 0;//set via command buffer dynamically
            outDepthStencilStateInfo->depthBoundsTestEnable = false;
            outDepthStencilStateInfo->minDepthBounds = 0;//we're not doing depth bounds testing, ignore
            outDepthStencilStateInfo->maxDepthBounds = 0;//we're not doing depth bounds testing, ignore
        }

        void VulkanShaderPipeline::move(VulkanShaderPipeline& from)
        {
            _graphicsCard = from._graphicsCard;
            std::swap(_pipeline, from._pipeline);
            _type = from._type;
        }
    } // vulkan
} // slag