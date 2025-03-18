#include "VulkanShaderPipeline.h"

#include <sstream>


namespace slag
{
    namespace vulkan
    {


        VulkanShaderPipeline::VulkanShaderPipeline(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            std::vector<VulkanShaderData> shaderStageData;
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages(moduleCount,VkPipelineShaderStageCreateInfo{});
            size_t vertexStageIndex = SIZE_MAX;
            size_t fragmentStageIndex = SIZE_MAX;

            for(size_t i=0; i< moduleCount; i++)
            {
                auto& module = modules[i];
                shaderStageData.emplace_back(module);

                auto& createInfo = shaderStages[i];
                createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                createInfo.stage = std::bit_cast<VkShaderStageFlagBits>(module.stage());
                createInfo.module = shaderStageData[i].shaderModule;
                createInfo.pName = "main";
                if(module.stage() == ShaderStageFlags::VERTEX)
                {
                    vertexStageIndex = i;
                }
                if(module.stage() == ShaderStageFlags::FRAGMENT)
                {
                    fragmentStageIndex = i;
                }

            }
            if(vertexStageIndex==SIZE_MAX || fragmentStageIndex==SIZE_MAX)
            {
                throw std::runtime_error("Must define both a vertex stage and fragment stage");
            }
            constructPipeline(descriptorGroups, descriptorGroupCount, properties, vertexDescription, frameBufferDescription, shaderStageData, shaderStages, vertexStageIndex);

        }

        VulkanShaderPipeline::VulkanShaderPipeline(ShaderModule** modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties,VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately):resources::Resource(destroyImmediately)
        {
            std::vector<VulkanShaderData> shaderStageData;
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages(moduleCount,VkPipelineShaderStageCreateInfo{});
            size_t vertexStageIndex = SIZE_MAX;
            size_t fragmentStageIndex = SIZE_MAX;

            for(size_t i=0; i< moduleCount; i++)
            {
                auto& module = modules[i];
                shaderStageData.emplace_back(*module);

                auto& createInfo = shaderStages[i];
                createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                createInfo.stage = std::bit_cast<VkShaderStageFlagBits>(module->stage());
                createInfo.module = shaderStageData[i].shaderModule;
                createInfo.pName = "main";
                if(module->stage() == ShaderStageFlags::VERTEX)
                {
                    vertexStageIndex = i;
                }
                if(module->stage() == ShaderStageFlags::FRAGMENT)
                {
                    fragmentStageIndex = i;
                }

            }
            if(vertexStageIndex==SIZE_MAX || fragmentStageIndex==SIZE_MAX)
            {
                throw std::runtime_error("Must define both a vertex stage and fragment stage");
            }
            constructPipeline(descriptorGroups, descriptorGroupCount, properties, vertexDescription, frameBufferDescription, shaderStageData, shaderStages, vertexStageIndex);
        }
        VulkanShaderPipeline::~VulkanShaderPipeline()
        {
            if(_layout)
            {
                smartDestroy();
            }
        }

        VulkanShaderPipeline::VulkanShaderPipeline(VulkanShaderPipeline&& from): resources::Resource(from._destroyImmediately)
        {
            move(std::move(from));
        }

        VulkanShaderPipeline& VulkanShaderPipeline::operator=(VulkanShaderPipeline&& from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanShaderPipeline::move(VulkanShaderPipeline&& from)
        {
            resources::Resource::move(from);
            _descriptorGroups.swap(from._descriptorGroups);
            _pushConstantRanges.swap(from._pushConstantRanges);
            std::swap(_pipeline,from._pipeline);
            std::swap(_layout,from._layout);
        }

        void VulkanShaderPipeline::constructPipeline(DescriptorGroup* const* descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties,
                                                     VertexDescription* vertexDescription, const FrameBufferDescription& frameBufferDescription, const std::vector<VulkanShaderData>& shaderStageData,
                                                     std::vector<VkPipelineShaderStageCreateInfo>& shaderStages, size_t vertexStageIndex)
        {//get descriptor groups via reflection
            std::unordered_map<size_t,std::vector<VulkanDescriptorGroup>> reflectedDescriptorGroups;
            size_t maxDescriptorGroup = descriptorGroupCount;
            bool hasDescriptorGroups = false;
            for(size_t i=0; i< shaderStageData.size(); i++)
            {
                auto& module = shaderStageData[i];
                auto& reflectModule = shaderStageData[i].reflectModule;
                uint32_t setCount = 0;
                auto result = spvReflectEnumerateDescriptorSets(&reflectModule,&setCount, nullptr);
                if(setCount > 0)
                {
                    hasDescriptorGroups = true;
                }
                for(size_t set = 0; set< setCount; set++)
                {
                    auto binding = reflectModule.descriptor_bindings[set];
                    //if we've passed an override, don't bother figuring all this out
                    if(binding.set < descriptorGroupCount)
                    {
                        continue;
                    }
                    else if(binding.set > maxDescriptorGroup)
                    {
                        maxDescriptorGroup = binding.set;
                    }
                    auto descriptorSet = spvReflectGetDescriptorSet(&reflectModule,binding.set,&result);
                    if(descriptorSet != nullptr)
                    {
                        std::vector<Descriptor> setDescriptors;
                        for(uint32_t descriptorIndex=0; descriptorIndex<descriptorSet->binding_count; descriptorIndex++)
                        {
                            auto desc = descriptorSet->bindings[descriptorIndex];
                            setDescriptors.push_back(Descriptor(desc->name,lib::BackEndLib::descriptorTypeFromSPV(desc->descriptor_type),desc->count,desc->binding,module.stageFlags));
                        }
                        if(!reflectedDescriptorGroups.contains(binding.set))
                        {
                            reflectedDescriptorGroups[binding.set] = std::vector<VulkanDescriptorGroup>();
                        }
                        auto& group = reflectedDescriptorGroups[binding.set];
                        group.push_back(VulkanDescriptorGroup(setDescriptors.data(),setDescriptors.size()));
                    }
                }
                uint32_t blockCount = 0;
                result = spvReflectEnumeratePushConstantBlocks(&reflectModule,&blockCount, nullptr);
                for(uint32_t blockIndex=0; blockIndex<blockCount; blockIndex++)
                {
                    auto& range = *spvReflectGetPushConstantBlock(&reflectModule,blockIndex,&result);
                    _pushConstantRanges.push_back(PushConstantRange{.stageFlags = module.stageFlags,.offset = range.offset,.size = range.size});
                    //TODO: acquire actual variables and assign them to the ranges
                }
            }

            //add descriptor groups to shader
            for(size_t i=0; i<=maxDescriptorGroup && hasDescriptorGroups; i++)
            {
                //if we provided an override for a group, use that
                if(i < descriptorGroupCount)
                {
                    _descriptorGroups.push_back(*static_cast<VulkanDescriptorGroup*>(descriptorGroups[i]));
                }
                    //otherwise, merge the groups we found from reflection and use that
                else
                {
                    auto& groups = reflectedDescriptorGroups[i];
                    std::vector<DescriptorGroup*> groupPointers(groups.size());
                    for(size_t j=0; j<groups.size(); j++)
                    {
                        groupPointers[j] = &groups[j];
                    }
                    auto descriptors = DescriptorGroup::combine(groupPointers.data(),groupPointers.size());
                    _descriptorGroups.push_back(VulkanDescriptorGroup(descriptors.data(), descriptors.size()));
                }
            }

/*************************************************************************/
            VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
            rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationInfo.pNext = nullptr;
            rasterizationInfo.flags = 0;
            rasterizationInfo.depthClampEnable = properties.rasterizationState.depthClampEnable;
            rasterizationInfo.rasterizerDiscardEnable = properties.rasterizationState.rasterizerDicardEnable;
            rasterizationInfo.polygonMode = VulkanLib::polygonMode(properties.rasterizationState.drawMode);
            rasterizationInfo.cullMode = VulkanLib::cullMode(properties.rasterizationState.culling);
            rasterizationInfo.frontFace = VulkanLib::frontFace(properties.rasterizationState.frontFacing);
            rasterizationInfo.depthBiasEnable = properties.rasterizationState.depthBiasEnable;
            rasterizationInfo.depthBiasConstantFactor = static_cast<float>(properties.rasterizationState.depthBiasConstantFactor);
            rasterizationInfo.depthBiasClamp = properties.rasterizationState.depthBiasClamp;
            rasterizationInfo.depthBiasSlopeFactor = properties.rasterizationState.depthBiasSlopeFactor;
            rasterizationInfo.lineWidth = properties.rasterizationState.lineWidth;


            VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
            multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleInfo.pNext = nullptr;
            multisampleInfo.flags = 0;
            multisampleInfo.rasterizationSamples = static_cast<VkSampleCountFlagBits>(properties.multiSampleState.rasterizationSamples);
            multisampleInfo.sampleShadingEnable = properties.multiSampleState.sampleShadingEnable;
            multisampleInfo.minSampleShading = properties.multiSampleState.minSampleShading == 0? 0: static_cast<float>(properties.multiSampleState.rasterizationSamples) / static_cast<float>(properties.multiSampleState.minSampleShading);
            //multisampleInfo.pSampleMask = 0;
            multisampleInfo.alphaToCoverageEnable = false;
            multisampleInfo.alphaToOneEnable = properties.multiSampleState.alphaToOneEnable;

            std::vector<VkPipelineColorBlendAttachmentState> attachmentStates(std::clamp(frameBufferDescription.colorTargetCount(),size_t(0),size_t(8)),VkPipelineColorBlendAttachmentState{});
            for(size_t i=0; i<attachmentStates.size(); i++)
            {
                auto& colorBlendAttachment = attachmentStates[i];
                auto& colorBlendDescription = properties.blendState.attachmentBlendStates[i];

                colorBlendAttachment.blendEnable = colorBlendDescription.blendingEnabled;
                colorBlendAttachment.srcColorBlendFactor = VulkanLib::blendFactor(colorBlendDescription.srcColorBlendFactor);
                colorBlendAttachment.dstColorBlendFactor = VulkanLib::blendFactor(colorBlendDescription.dstColorBlendFactor);
                colorBlendAttachment.colorBlendOp = VulkanLib::blendOp(colorBlendDescription.colorBlendOperation);
                colorBlendAttachment.srcAlphaBlendFactor = VulkanLib::blendFactor(colorBlendDescription.srcAlphaBlendFactor);
                colorBlendAttachment.dstAlphaBlendFactor = VulkanLib::blendFactor(colorBlendDescription.dstAlphaBlendFactor);
                colorBlendAttachment.alphaBlendOp = VulkanLib::blendOp(colorBlendDescription.alphaBlendOperation);
                colorBlendAttachment.colorWriteMask = VulkanLib::colorComponents(colorBlendDescription.colorWriteMask);
            }

            VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {};
            colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendingInfo.pNext = nullptr;
            colorBlendingInfo.flags = 0;
            colorBlendingInfo.logicOpEnable = properties.blendState.logicOperationEnable;
            colorBlendingInfo.logicOp = VulkanLib::logicOp(properties.blendState.logicalOperation);
            colorBlendingInfo.attachmentCount = attachmentStates.size();
            colorBlendingInfo.pAttachments = attachmentStates.data();

            VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
            depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencilInfo.pNext = nullptr;
            depthStencilInfo.flags = 0;
            depthStencilInfo.depthTestEnable = properties.depthStencilState.depthTestEnable;
            depthStencilInfo.depthWriteEnable = properties.depthStencilState.depthWriteEnable;
            depthStencilInfo.depthCompareOp = VulkanLib::compareOp(properties.depthStencilState.depthCompareOperation);
            depthStencilInfo.stencilTestEnable = properties.depthStencilState.stencilTestEnable;
            depthStencilInfo.front.failOp = VulkanLib::stencilOp(properties.depthStencilState.front.failOp);
            depthStencilInfo.front.passOp = VulkanLib::stencilOp(properties.depthStencilState.front.passOp);
            depthStencilInfo.front.depthFailOp = VulkanLib::stencilOp(properties.depthStencilState.front.depthFailOp);
            depthStencilInfo.front.compareOp = VulkanLib::compareOp(properties.depthStencilState.front.compareOp);
            depthStencilInfo.front.compareMask = properties.depthStencilState.stencilReadMask;//I think only lest significant digits need to be set
            depthStencilInfo.front.writeMask = properties.depthStencilState.stencilWriteMask;//I think only lest significant digits need to be set
            depthStencilInfo.front.reference = 0;//set via command buffer dynamically
            depthStencilInfo.back.failOp = VulkanLib::stencilOp(properties.depthStencilState.back.failOp);
            depthStencilInfo.back.passOp = VulkanLib::stencilOp(properties.depthStencilState.back.passOp);
            depthStencilInfo.back.depthFailOp = VulkanLib::stencilOp(properties.depthStencilState.back.depthFailOp);
            depthStencilInfo.back.compareOp = VulkanLib::compareOp(properties.depthStencilState.back.compareOp);
            depthStencilInfo.back.compareMask = properties.depthStencilState.stencilReadMask;//I think only lest significant digits need to be set
            depthStencilInfo.back.writeMask = properties.depthStencilState.stencilWriteMask;//I think only lest significant digits need to be set
            depthStencilInfo.back.reference = 0;//set via command buffer dynamically
            depthStencilInfo.depthBoundsTestEnable = false;
            depthStencilInfo.minDepthBounds = 0;//we're not doing depth bounds testing, ignore
            depthStencilInfo.maxDepthBounds = 0;//we're not doing depth bounds testing, ignore


            std::vector<VkVertexInputAttributeDescription> attributes;
            std::vector<VkVertexInputBindingDescription> bindingDescriptions;
            //if we have provided a description for the vertex, use that
            if(vertexDescription)
            {
                attributes.resize(vertexDescription->attributeCount());
                bindingDescriptions.resize(vertexDescription->attributeChannels());
                size_t attIndex = 0;
                for (size_t channel = 0; channel < vertexDescription->attributeChannels(); channel++)
                {
                    uint32_t stride = 0;
                    for (size_t attribute = 0; attribute < vertexDescription->attributeCount(channel); attribute++)
                    {
                        auto& attr = attributes[attIndex];
                        auto& description = vertexDescription->attribute(channel, attribute);
                        attr.location = attribute;
                        attr.binding = channel;
                        attr.format = VulkanLib::graphicsType(description.dataType());
                        if(attr.format == VK_FORMAT_UNDEFINED)
                        {
                            throw std::runtime_error("Unable to convert graphicsType type into underlying API type");
                        }
                        attr.offset = description.offset();
                        attIndex++;
                        size_t end = attr.offset + GraphicsTypes::typeSize(description.dataType());
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
            }
                //otherwise get vertex info from reflection
            else
            {
                auto& vertexModule = shaderStageData[vertexStageIndex].reflectModule;
                uint32_t offset = 0;

                for(uint32_t i=0; i< vertexModule.input_variable_count; i++)
                {
                    auto inputVar = vertexModule.input_variables[i];
                    if(inputVar->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
                    {
                        continue;
                    }
                    auto type = lib::BackEndLib::graphicsTypeFromSPV(inputVar->format);
                    if(type == GraphicsTypes::UNKNOWN)
                    {
                        throw std::runtime_error("Vertex Shader Module contains vertex attribute of unknown type");
                    }
                    attributes.emplace_back(inputVar->location,0,static_cast<VkFormat>(inputVar->format),offset);
                    offset+= GraphicsTypes::typeSize(type);
                }
                bindingDescriptions.emplace_back(0,offset,VK_VERTEX_INPUT_RATE_VERTEX);

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
            std::vector<VkPushConstantRange> pushConstantRanges(_pushConstantRanges.size());
            for(size_t i=0; i < _descriptorGroups.size(); i++)
            {
                layouts[i] = _descriptorGroups[i].layout();
            }
            for(size_t i=0; i < _pushConstantRanges.size(); i++)
            {
                auto& range = pushConstantRanges[i];
                auto& templ = _pushConstantRanges[i];
                range.size = templ.size;
                range.offset = templ.offset;
                range.stageFlags = std::bit_cast<VkShaderStageFlags>(templ.stageFlags);
            }
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.pNext = nullptr;
            pipelineLayoutInfo.flags = 0;
            pipelineLayoutInfo.setLayoutCount = layouts.size();
            pipelineLayoutInfo.pSetLayouts = layouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
            pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

            if(vkCreatePipelineLayout(static_cast<VkDevice>(VulkanLib::card()->device()),&pipelineLayoutInfo, nullptr,&_layout) != VK_SUCCESS)
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
            pipelineInfo.layout = _layout;
            pipelineInfo.renderPass = VK_NULL_HANDLE;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.pDynamicState = &dynamicInfo;

            auto colorAttachments =std::vector<VkFormat>(frameBufferDescription.colorTargetCount());
            for(int i=0; i< frameBufferDescription.colorTargetCount(); i++)
            {
                colorAttachments[i] = VulkanLib::format(frameBufferDescription.colorFormat(i)).format;
            }

            VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                    .pNext = nullptr,
                    .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
                    .pColorAttachmentFormats = colorAttachments.data(),
                    .depthAttachmentFormat = VulkanLib::format(frameBufferDescription.depthFormat()).format,
                    .stencilAttachmentFormat = Pixels::hasStencilComponent(frameBufferDescription.depthFormat()) ?  VulkanLib::format(frameBufferDescription.depthFormat()).format : VK_FORMAT_UNDEFINED
            };

            pipelineInfo.pNext = &pipelineRenderingCreateInfo;

            auto result = vkCreateGraphicsPipelines(static_cast<VkDevice>(VulkanLib::card()->device()), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);

            auto pipeline = _pipeline;
            auto pipelineLayout = _layout;

            _disposeFunction = [=]()
            {
                vkDestroyPipeline(VulkanLib::card()->device(),static_cast<VkPipeline>(pipeline), nullptr);
                vkDestroyPipelineLayout(VulkanLib::card()->device(),static_cast<VkPipelineLayout>(pipelineLayout),nullptr);
            };

            assert(result == VK_SUCCESS && "Unable to create shader pipeline");
        }

        size_t VulkanShaderPipeline::descriptorGroupCount()
        {
            return _descriptorGroups.size();
        }

        DescriptorGroup* VulkanShaderPipeline::descriptorGroup(size_t index)
        {
            return &_descriptorGroups.at(index);
        }

        DescriptorGroup* VulkanShaderPipeline::operator[](size_t index)
        {
            return &_descriptorGroups[index];
        }

        size_t VulkanShaderPipeline::pushConstantRangeCount()
        {
            return _pushConstantRanges.size();
        }

        PushConstantRange VulkanShaderPipeline::pushConstantRange(size_t index)
        {
            return _pushConstantRanges[index];
        }

        VkPipeline VulkanShaderPipeline::pipeline() const
        {
            return _pipeline;
        }

        VkPipelineLayout VulkanShaderPipeline::layout() const
        {
            return _layout;
        }

    } // vulkan
} // slag