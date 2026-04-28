#include "VulkanShaderPipeline.h"

#include "VulkanBackend.h"
#include "VulkanShaderModule.h"
#include "slag/exceptions/InvalidShaderVertexBindingError.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanShaderPipeline::VulkanShaderPipeline(
            VulkanGraphicsCard* graphicsCard,
            const VertexDescription& vertexDescription,
            ShaderModule* vertexShader,
            ShaderModule* fragmentShader,
            PipelineInputMapping* inputBindings,
            const PipelineState& pipelineState,
            const FramebufferDescription& framebufferDescription)
        {
            SLAG_ASSERT(vertexShader != nullptr && vertexShader->metaData().type() == ShaderType::VERTEX && "provided vertex shader is not a vertex shader");
            SLAG_ASSERT(fragmentShader != nullptr && fragmentShader->metaData().type() == ShaderType::FRAGMENT && "provided fragment shader is not a fragment shader");
            SLAG_ASSERT(vertexShader->graphicsCard() == graphicsCard && fragmentShader->graphicsCard() == graphicsCard && "All shader modules must be on the graphics card creating the shader pipeline");
            _graphicsCard = graphicsCard;


            VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
            initNativeRasterizationInfo(pipelineState.rasterizationState,&rasterizationCreateInfo);

            VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
            initMultisampleInfo(pipelineState.multiSampleState,&multisampleCreateInfo);

            VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
            auto blendData = initColorAttachmentInfo(pipelineState.blendState,framebufferDescription,&colorBlendCreateInfo);

            VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
            initDepthAttachmentInfo(pipelineState.depthStencilState,&depthStencilInfo);

            std::vector<std::string> foundNames;
            auto& vertexMeta = vertexShader->metaData();
            foundNames.reserve(vertexMeta.inputVariableCount());
            std::vector<VkVertexInputAttributeDescription> attributes;
            std::vector<VkVertexInputBindingDescription> bindingDescriptions;


            for (auto i=0u; i< vertexDescription.bindingCount(); ++i)
            {
                auto& binding = vertexDescription[i];
                for (auto attribute = 0u; attribute < binding.attributeCount(); ++attribute)
                {
                    auto& curAttribute = binding[attribute];
                    bool foundMatch = false;
                    uint32_t vulkanLocation = 0;
                    for (auto input=0u; input< vertexMeta.inputVariableCount(); ++input)
                    {
                        auto& curInput = vertexMeta.inputVariable(input);
                        if (curInput.name() == curAttribute.name())
                        {
                            foundMatch = true;
                            vulkanLocation = curInput.location();
                            attributes.emplace_back(vulkanLocation,binding.bindingIndex(),VulkanBackend::nativeFormat(curAttribute.loadAs()).format,curAttribute.offset());
                            break;
                        }
                    }
                    if (!foundMatch)
                    {
                        throw InvalidShaderVertexBindingError("Expected vertex shader to have vertex attribute \""+curAttribute.name()+"\"");
                    }
                }

                bindingDescriptions.emplace_back(binding.bindingIndex(),binding.stride(),VK_VERTEX_INPUT_RATE_VERTEX);
            }
            if (attributes.size() != vertexMeta.inputVariableCount())
            {
                throw InvalidShaderVertexBindingError("Mismatch between vertex shader inputs and provided vertex description");
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
            std::vector<VkDescriptorSetAndBindingMappingEXT> bindingMappings;
            if (inputBindings!=nullptr)
            {
                _bindings = *inputBindings;
                for (auto i=0u; i< inputBindings->inputCount(); i++)
                {
                    auto& binding = inputBindings->input(i);
                    switch (binding.type())
                    {
                    case PipelineInputType::CONSTANT_RANGE:
                        {
                            auto constRange = binding.constantRange();
                            VkDescriptorSetAndBindingMappingEXT constRangeInput
                            {
                                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
                                .pNext = nullptr,
                                .descriptorSet = constRange->bindGroupIndex,
                                .firstBinding = constRange->binding,
                                .bindingCount = 1,
                                .resourceMask = VK_SPIRV_RESOURCE_TYPE_ALL_EXT,
                                .source = VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_DATA_EXT,
                            };
                            constRangeInput.sourceData.pushDataOffset = binding.location();
                            bindingMappings.push_back(constRangeInput);
                        }
                        break;
                    case PipelineInputType::DESCRIPTOR:
                        {
                            auto descriptor = binding.descriptor();
                            VkDescriptorSetAndBindingMappingEXT descriptorInput
                            {
                                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
                                .pNext = nullptr,
                                .descriptorSet = descriptor->bindGroupIndex,
                                .firstBinding = descriptor->binding,
                                .bindingCount = 1,
                                .resourceMask = VK_SPIRV_RESOURCE_TYPE_ALL_EXT,
                                .source = VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_ADDRESS_EXT,
                            };
                            descriptorInput.sourceData.pushAddressOffset = binding.location();
                            bindingMappings.push_back(descriptorInput);
                        }
                        break;
                    case PipelineInputType::DESCRIPTOR_TABLE:
                        {
                            auto table = binding.descriptorTable();
                            for (auto range = 0u; range < table->rangeCount(); range++)
                            {
                                auto curRange = table->descriptorRange(range);
                                VkDescriptorSetAndBindingMappingEXT rangeInput
                                {
                                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
                                    .pNext = nullptr,
                                    .descriptorSet = curRange.bindGroupIndex,
                                    .firstBinding = curRange.firstBinding,
                                    .bindingCount = curRange.bindingCount,
                                    .resourceMask = VK_SPIRV_RESOURCE_TYPE_ALL_EXT,
                                    .source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_PUSH_INDEX_EXT,
                                };
                                switch (curRange.type)
                                {
                                case DescriptorRangeType::UNIFORM_BUFFER:
                                case DescriptorRangeType::READONLY_RESOURCE:
                                case DescriptorRangeType::UNORDERED_ACCESS_RESOURCE:
                                    rangeInput.sourceData.pushIndex.heapOffset = curRange.offsetInDescriptorsFromTableStart * _graphicsCard->descriptorHeapDetails().resourceDescriptorIncrementSize;
                                    break;
                                case DescriptorRangeType::SAMPLER:
                                    rangeInput.sourceData.pushIndex.heapOffset = curRange.offsetInDescriptorsFromTableStart * _graphicsCard->descriptorHeapDetails().samplerDescriptorIncrementSize;
                                    break;
                                }

                                rangeInput.sourceData.pushIndex.heapIndexStride = 1;
                                rangeInput.sourceData.pushIndex.pushOffset = binding.location();
                                bindingMappings.push_back(rangeInput);
                            }
                        }
                        break;
                    }
                }
            }
            mappings.mappingCount = bindingMappings.size();
            mappings.pMappings = bindingMappings.data();

            VkPipelineShaderStageCreateInfo shaderStages[2];
            shaderStages[0] =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = &mappings,
                .flags = 0,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = static_cast<VulkanShaderModule*>(vertexShader)->nativeModule(),
                .pName = "main",
                .pSpecializationInfo = nullptr
            };
            shaderStages[1] =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = &mappings,
                .flags = 0,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = static_cast<VulkanShaderModule*>(fragmentShader)->nativeModule(),
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

        const PipelineInputMapping& VulkanShaderPipeline::bindings()
        {
            return _bindings;
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
            std::swap(_bindings,from._bindings);
            _graphicsCard = from._graphicsCard;
            std::swap(_pipeline, from._pipeline);
            _type = from._type;
        }
    } // vulkan
} // slag