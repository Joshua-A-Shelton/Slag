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

        VulkanShaderPipeline::VulkanShaderPipeline(ShaderCode** shaders, uint32_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription, DescriptorIdentity(*identify)(const DescriptorIdentityParameters&,void*), void* identifyData)
        {
            _pipelineType = PipelineType::GRAPHICS;
            _xthreads = 0;
            _ythreads = 0;
            _zthreads = 0;
// get the reflection data *********************************************************************************************

            ShaderPipelineMetaData pipelineMetadata(shaders, shaderCount);

            if (vertexDescription.attributeCount() != pipelineMetadata.vertexInputCount())
            {
                throw std::invalid_argument("vertexDescription must have same number of vertex attributes as the shader expects: "+std::to_string(pipelineMetadata.vertexInputCount()));
            }
// reorder descriptors if required
            std::vector<std::unordered_map<uint32_t,DescriptorIdentity>> indexRemappings(pipelineMetadata.descriptorGroupsCount());
            std::vector<std::vector<bool>> mappedIndexes(pipelineMetadata.descriptorGroupsCount());
            if (identify!=nullptr)
            {
                for (auto i = 0u; i < pipelineMetadata.descriptorGroupsCount(); i++)
                {
                    auto& descriptorGroup = pipelineMetadata.descriptorGroup(i);
                    auto& groupRemap = indexRemappings[i];
                    auto& indexes = mappedIndexes[i];
                    indexes.resize(descriptorGroup.bindingCount(),false);
                    for (auto j = 0u; j < descriptorGroup.bindingCount(); j++)
                    {
                        auto& binding = descriptorGroup.descriptorBinding(j);
                        auto& curDescriptor = binding.descriptor();
                        DescriptorIdentityParameters parameters
                        {
                            .language = ShaderCode::CodeLanguage::SPIRV,
                            .descriptorGroupIndex = i,
                            .descriptor = &curDescriptor,
                        };
                        auto identified = identify(parameters,identifyData);
                        if (identified.index >= indexes.size())
                        {
                            throw std::runtime_error("Remapped index beyond range of descriptor count");
                        }
                        groupRemap.insert(std::pair(j,identified));
                        if (indexes[identified.index] == true)
                        {
                            throw std::runtime_error("Multiple descriptors are being mapped to the same index");
                        }
                        indexes[identified.index] = true;
                    }
                }
            }
            else
            {
                for (auto i = 0u; i < pipelineMetadata.descriptorGroupsCount(); i++)
                {
                    auto& descriptorGroup = pipelineMetadata.descriptorGroup(i);
                    auto& groupRemap = indexRemappings[i];
                    auto& indexes = mappedIndexes[i];
                    indexes.resize(descriptorGroup.bindingCount(),false);
                    for (uint32_t j = 0u; j < descriptorGroup.bindingCount(); j++)
                    {
                        auto& binding = descriptorGroup.descriptorBinding(j);
                        groupRemap.insert(std::pair(j,DescriptorIdentity(binding.descriptor().name(),binding.bindingId())));
                        if (indexes[binding.bindingId()] == true)
                        {
                            throw std::runtime_error("Multiple descriptors are being mapped to the same index");
                        }
                        indexes[binding.bindingId()] = true;
                    }
                }
            }

            for (int i=0; i<mappedIndexes.size(); i++)
            {
                auto& indexCheck = mappedIndexes[i];
                for (auto j=0; j < indexCheck.size(); j++)
                {
                    if (indexCheck[j] == false)
                    {
                        throw std::runtime_error("An empty descriptor index exists for a descriptor group");
                    }
                }
            }

// assemble shader pipeline
            for (auto i=0; i<pipelineMetadata.uniformBufferLayoutCount(); i++)
            {
                auto& uniformBufferLayout = pipelineMetadata.uniformBufferLayout(i);
                auto remappedDescriptorIndex = indexRemappings[uniformBufferLayout.descriptorGroupIndex()].at(uniformBufferLayout.descriptorIndex());
                auto uniformBufferDescriptorGroup = _uniformBufferLayouts.find(uniformBufferLayout.descriptorGroupIndex());
                if (uniformBufferDescriptorGroup == _uniformBufferLayouts.end())
                {
                    uniformBufferDescriptorGroup = _uniformBufferLayouts.insert(std::pair(uniformBufferLayout.descriptorGroupIndex(),std::unordered_map<uint32_t,BufferLayout>())).first;
                }
                uniformBufferDescriptorGroup->second[remappedDescriptorIndex.index] = uniformBufferLayout.bufferLayout();
            }
            for (auto i=0; i<pipelineMetadata.storageBufferLayoutCount(); i++)
            {
                auto& storageBufferLayout = pipelineMetadata.storageBufferLayout(i);
                auto remappedDescriptorIndex = indexRemappings[storageBufferLayout.descriptorGroupIndex()].at(storageBufferLayout.descriptorIndex());
                auto storageBufferDescriptorGroup = _storageBufferLayouts.find(storageBufferLayout.descriptorGroupIndex());
                if (storageBufferDescriptorGroup == _storageBufferLayouts.end())
                {
                    storageBufferDescriptorGroup = _storageBufferLayouts.insert(std::pair(storageBufferLayout.descriptorGroupIndex(),std::unordered_map<uint32_t,BufferLayout>())).first;
                }
                storageBufferDescriptorGroup->second[remappedDescriptorIndex.index] = storageBufferLayout.bufferLayout();
            }

            for (auto i=0; i<pipelineMetadata.texelBufferDescriptionCount(); i++)
            {
                auto& texelBufferDescription = pipelineMetadata.texelBufferDescription(i);
                auto remappedDescriptorIndex = indexRemappings[texelBufferDescription.descriptorGroupIndex()].at(texelBufferDescription.descriptorIndex());
                auto texelBufferDescriptorGroup = _texelBufferDescriptions.find(texelBufferDescription.descriptorGroupIndex());
                if (texelBufferDescriptorGroup == _texelBufferDescriptions.end())
                {
                    texelBufferDescriptorGroup = _texelBufferDescriptions.insert(std::pair(texelBufferDescription.descriptorGroupIndex(),std::unordered_map<uint32_t,TexelBufferDescription>())).first;
                }
                texelBufferDescriptorGroup->second.insert(std::pair(remappedDescriptorIndex.index,texelBufferDescription.bufferDescription())) ;
            }


            _descriptorGroups.resize(pipelineMetadata.descriptorGroupsCount());
            for (auto i = 0; i < pipelineMetadata.descriptorGroupsCount(); i++)
            {
                auto& descriptors = pipelineMetadata.descriptorGroup(i);
                auto& groupRemappings = indexRemappings[i];

                _descriptorGroups[i] = VulkanDescriptorGroup(descriptors,groupRemappings);
            }
            if (pipelineMetadata.pushConstantLayout().type() != GraphicsType::UNKNOWN)
            {
                _pushConstants = std::make_unique<BufferLayout>(pipelineMetadata.pushConstantLayout());
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
            std::vector<VkVertexInputBindingDescription> bindingDescriptions(vertexDescription.attributeChannels());
            size_t attIndex = 0;
            for (size_t channel = 0; channel < vertexDescription.attributeChannels(); channel++)
            {
                uint32_t stride = 0;
                for (size_t attribute = 0; attribute < vertexDescription.attributeCount(channel); attribute++)
                {
                    auto& reflectedAttr = pipelineMetadata.vertexInput(attIndex);
                    //TODO: this isn't correct. I need to map it to the reflected ones instead of assuming they're in the same order
                    auto& description = vertexDescription.attribute(channel, attribute);
                    for (uint32_t arrayIndex = 0; arrayIndex < reflectedAttr.arrayLength(); arrayIndex++)
                    {
                        SLAG_ASSERT(reflectedAttr.type() == description.dataType() && reflectedAttr.arrayLength() == description.arrayLength() && "Mismatch between vertex attributes and vertex description");
                        VkVertexInputAttributeDescription attr;
                        attr.location = reflectedAttr.inputID()+arrayIndex;
                        attr.binding = channel;
                        attr.format = VulkanBackend::vulkanizedGraphicsType(description.dataType());
                        if(attr.format == VK_FORMAT_UNDEFINED)
                        {
                            throw std::runtime_error("Unable to convert graphicsType type into underlying API type");
                        }
                        attr.offset = description.offset() + (graphicsTypeSize(reflectedAttr.type()) * arrayIndex);

                        size_t end = attr.offset + graphicsTypeSize(description.dataType());
                        if (end > stride)
                        {
                            stride = end;
                        }

                        attributes.push_back(attr);
                    }
                    attIndex++;
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

            if (pipelineMetadata.pushConstantLayout().type()!=GraphicsType::UNKNOWN)
            {
                _pushConstants = std::make_unique<BufferLayout>(pipelineMetadata.pushConstantLayout());
            }

            VkPushConstantRange pushConstantRange = {};
            if (_pushConstants != nullptr)
            {
                pushConstantRange.offset = _pushConstants->offset();
                pushConstantRange.size = _pushConstants->size();
                pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.pNext = nullptr;
            pipelineLayoutInfo.flags = 0;
            pipelineLayoutInfo.setLayoutCount = layouts.size();
            pipelineLayoutInfo.pSetLayouts = layouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = _pushConstants==nullptr? 0:1;
            pipelineLayoutInfo.pPushConstantRanges = _pushConstants==nullptr? nullptr:&pushConstantRange;

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
            pipelineInfo.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

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

        VulkanShaderPipeline::VulkanShaderPipeline(ShaderCode* computeCode, DescriptorIdentity(*identify)(const DescriptorIdentityParameters&,void*), void* identifyData)
        {
            SLAG_ASSERT(computeCode != nullptr && "compute code must not be null");
            SLAG_ASSERT(computeCode->stage() == ShaderStageFlags::COMPUTE && "compute code must be of type compute");
            _pipelineType = PipelineType::COMPUTE;
            auto reflectionData = computeCode->metaData();
            _xthreads = reflectionData->xComputeThreads();
            _ythreads = reflectionData->yComputeThreads();
            _zthreads = reflectionData->zComputeThreads();

            auto pipelineMetadata = ShaderPipelineMetaData(computeCode,1);

            std::vector<std::unordered_map<uint32_t,DescriptorIdentity>> indexRemappings(pipelineMetadata.descriptorGroupsCount());
            std::vector<std::vector<bool>> mappedIndexes(pipelineMetadata.descriptorGroupsCount());
            if (identify!=nullptr)
            {
                for (auto i = 0u; i < pipelineMetadata.descriptorGroupsCount(); i++)
                {
                    auto& descriptorGroup = pipelineMetadata.descriptorGroup(i);
                    auto& groupRemap = indexRemappings[i];
                    auto& indexes = mappedIndexes[i];
                    indexes.resize(descriptorGroup.bindingCount(),false);
                    for (auto j = 0u; j < descriptorGroup.bindingCount(); j++)
                    {
                        auto& binding = descriptorGroup.descriptorBinding(j);
                        auto& curDescriptor = binding.descriptor();
                        DescriptorIdentityParameters parameters
                        {
                            .language = ShaderCode::CodeLanguage::SPIRV,
                            .descriptorGroupIndex = i,
                            .descriptor = &curDescriptor,
                        };
                        auto identified = identify(parameters,identifyData);
                        if (identified.index >= indexes.size())
                        {
                            throw std::runtime_error("Remapped index beyond range of descriptor count");
                        }
                        groupRemap.insert(std::pair(j,identified));
                        if (indexes[identified.index] == true)
                        {
                            throw std::runtime_error("Multiple descriptors are being mapped to the same index");
                        }
                        indexes[identified.index] = true;
                    }
                }
            }
            else
            {
                for (auto i = 0u; i < pipelineMetadata.descriptorGroupsCount(); i++)
                {
                    auto& descriptorGroup = pipelineMetadata.descriptorGroup(i);
                    auto& groupRemap = indexRemappings[i];
                    auto& indexes = mappedIndexes[i];
                    indexes.resize(descriptorGroup.bindingCount(),false);
                    for (uint32_t j = 0u; j < descriptorGroup.bindingCount(); j++)
                    {
                        auto& binding = descriptorGroup.descriptorBinding(j);
                        groupRemap.insert(std::pair(j,DescriptorIdentity(binding.descriptor().name(),binding.bindingId())));
                        if (indexes[binding.bindingId()] == true)
                        {
                            throw std::runtime_error("Multiple descriptors are being mapped to the same index");
                        }
                        indexes[binding.bindingId()] = true;
                    }
                }
            }

            for (int i=0; i<mappedIndexes.size(); i++)
            {
                auto& indexCheck = mappedIndexes[i];
                for (auto j=0; j < indexCheck.size(); j++)
                {
                    if (indexCheck[j] == false)
                    {
                        throw std::runtime_error("An empty descriptor index exists for a descriptor group");
                    }
                }
            }

            for (auto i=0; i<pipelineMetadata.uniformBufferLayoutCount(); i++)
            {
                auto& uniformBufferLayout = pipelineMetadata.uniformBufferLayout(i);
                auto remappedDescriptorIndex = indexRemappings[uniformBufferLayout.descriptorGroupIndex()].at(uniformBufferLayout.descriptorIndex());
                auto uniformBufferDescriptorGroup = _uniformBufferLayouts.find(uniformBufferLayout.descriptorGroupIndex());
                if (uniformBufferDescriptorGroup == _uniformBufferLayouts.end())
                {
                    uniformBufferDescriptorGroup = _uniformBufferLayouts.insert(std::pair(uniformBufferLayout.descriptorGroupIndex(),std::unordered_map<uint32_t,BufferLayout>())).first;
                }
                uniformBufferDescriptorGroup->second[remappedDescriptorIndex.index] = uniformBufferLayout.bufferLayout();
            }
            for (auto i=0; i<pipelineMetadata.storageBufferLayoutCount(); i++)
            {
                auto& storageBufferLayout = pipelineMetadata.storageBufferLayout(i);
                auto remappedDescriptorIndex = indexRemappings[storageBufferLayout.descriptorGroupIndex()].at(storageBufferLayout.descriptorIndex());
                auto storageBufferDescriptorGroup = _storageBufferLayouts.find(storageBufferLayout.descriptorGroupIndex());
                if (storageBufferDescriptorGroup == _storageBufferLayouts.end())
                {
                    storageBufferDescriptorGroup = _storageBufferLayouts.insert(std::pair(storageBufferLayout.descriptorGroupIndex(),std::unordered_map<uint32_t,BufferLayout>())).first;
                }
                storageBufferDescriptorGroup->second[remappedDescriptorIndex.index] = storageBufferLayout.bufferLayout();
            }

            for (auto i=0; i<pipelineMetadata.texelBufferDescriptionCount(); i++)
            {
                auto& texelBufferDescription = pipelineMetadata.texelBufferDescription(i);
                auto remappedDescriptorIndex = indexRemappings[texelBufferDescription.descriptorGroupIndex()].at(texelBufferDescription.descriptorIndex());
                auto texelBufferDescriptorGroup = _texelBufferDescriptions.find(texelBufferDescription.descriptorGroupIndex());
                if (texelBufferDescriptorGroup == _texelBufferDescriptions.end())
                {
                    texelBufferDescriptorGroup = _texelBufferDescriptions.insert(std::pair(texelBufferDescription.descriptorGroupIndex(),std::unordered_map<uint32_t,TexelBufferDescription>())).first;
                }
                texelBufferDescriptorGroup->second.insert(std::pair(remappedDescriptorIndex.index,texelBufferDescription.bufferDescription())) ;
            }
            _descriptorGroups.resize(pipelineMetadata.descriptorGroupsCount());
            for (auto i = 0; i < pipelineMetadata.descriptorGroupsCount(); i++)
            {
                auto& descriptors = pipelineMetadata.descriptorGroup(i);
                auto& groupRemappings = indexRemappings[i];

                _descriptorGroups[i] = VulkanDescriptorGroup(descriptors,groupRemappings);
            }
            if (pipelineMetadata.pushConstantLayout().type() != GraphicsType::UNKNOWN)
            {
                _pushConstants = std::make_unique<BufferLayout>(pipelineMetadata.pushConstantLayout());
            }

            VulkanShaderModule shaderModule(computeCode);

            VkPipelineShaderStageCreateInfo computeStageCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_COMPUTE_BIT,.module = shaderModule.shaderModule,.pName = "main"};

            std::vector<VkDescriptorSetLayout> layouts(_descriptorGroups.size());

            for(size_t i=0; i < _descriptorGroups.size(); i++)
            {
                layouts[i] = _descriptorGroups[i].layout();
            }

            if (pipelineMetadata.pushConstantLayout().type()!=GraphicsType::UNKNOWN)
            {
                _pushConstants = std::make_unique<BufferLayout>(pipelineMetadata.pushConstantLayout());
            }

            VkPushConstantRange pushConstantRange = {};
            if (_pushConstants != nullptr)
            {
                pushConstantRange.offset = _pushConstants->offset();
                pushConstantRange.size = _pushConstants->size();
                pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.pNext = nullptr;
            pipelineLayoutInfo.pSetLayouts = layouts.data();
            pipelineLayoutInfo.setLayoutCount = layouts.size();
            pipelineLayoutInfo.pushConstantRangeCount = _pushConstants==nullptr? 0:1;
            pipelineLayoutInfo.pPushConstantRanges = _pushConstants==nullptr? nullptr:&pushConstantRange;

            if(vkCreatePipelineLayout(static_cast<VkDevice>(VulkanGraphicsCard::selected()->device()),&pipelineLayoutInfo, nullptr,&_pipelineLayout) != VK_SUCCESS)
            {
                throw std::runtime_error("Unable to create shader pipeline layout");
            }

            VkComputePipelineCreateInfo computePipelineCreateInfo{};
            computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            computePipelineCreateInfo.pNext = nullptr;
            computePipelineCreateInfo.layout = _pipelineLayout;
            computePipelineCreateInfo.stage = computeStageCreateInfo;
            computePipelineCreateInfo.flags = VK_PIPELINE_CREATE_DISPATCH_BASE | VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

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

        BufferLayout* VulkanShaderPipeline::pushConstants()
        {
            return _pushConstants.get();
        }

        VertexDescription* VulkanShaderPipeline::vertexDescription()
        {
            return _vertexDescription.get();
        }

        DescriptorGroup* VulkanShaderPipeline::descriptorGroup(uint32_t index)
        {
            return &_descriptorGroups.at(index);
        }

        DescriptorGroup* VulkanShaderPipeline::operator[](uint32_t index)
        {
            return &_descriptorGroups[index];
        }

        BufferLayout* VulkanShaderPipeline::uniformBufferLayout(uint32_t descriptorGroup,uint32_t descriptorBinding)
        {
            auto group = _uniformBufferLayouts.find(descriptorGroup);
            if(group == _uniformBufferLayouts.end())
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

        BufferLayout* VulkanShaderPipeline::storageBufferLayout(uint32_t descriptorGroup, uint32_t descriptorBinding)
        {
            auto group = _storageBufferLayouts.find(descriptorGroup);
            if(group == _storageBufferLayouts.end())
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
            _vertexDescription.swap(from._vertexDescription);
            std::swap(_descriptorGroups,from._descriptorGroups);
            _descriptorGroups.swap(from._descriptorGroups);
            _pushConstants.swap(from._pushConstants);
            _uniformBufferLayouts.swap(from._uniformBufferLayouts);
            _storageBufferLayouts.swap(from._storageBufferLayouts);
            _xthreads = from._xthreads;
            _ythreads = from._ythreads;
            _zthreads = from._zthreads;
        }
    } // vulkan
} // slag
