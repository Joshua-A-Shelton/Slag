#include "VulkanShader.h"
#include "VulkanLib.h"
#include "VulkanTexture.h"
#include <stdexcept>
#include <spirv_reflect.h>
#include <algorithm>

namespace slag
{
    namespace vulkan
    {
        VulkanShader::~VulkanShader()
        {
            if(_pipeline)
            {
                smartDestroy();
            }
        }

        VulkanShader::VulkanShader(const std::vector<char> &vertexCode, const std::vector<char> &fragmentCode, FramebufferDescription& framebufferDescription, VertexDescription* vertexDescription)
        {
            VkShaderModuleCreateInfo createVertexInfo = {};
            createVertexInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createVertexInfo.codeSize = vertexCode.size();
            createVertexInfo.pCode = reinterpret_cast<const uint32_t*>(vertexCode.data());

            VkShaderModule vshaderModule = nullptr;
            if(vkCreateShaderModule(VulkanLib::graphicsCard()->device(),&createVertexInfo, nullptr,&vshaderModule)!= VK_SUCCESS)
            {
                throw std::runtime_error("Unable to use vertex shader");
            }

            VkShaderModuleCreateInfo createFragmentInfo = {};
            createFragmentInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createFragmentInfo.codeSize = fragmentCode.size();
            createFragmentInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentCode.data());

            VkShaderModule fshaderModule = nullptr;
            if(vkCreateShaderModule(VulkanLib::graphicsCard()->device(),&createFragmentInfo, nullptr,&fshaderModule)!= VK_SUCCESS)
            {
                throw std::runtime_error("Unable to use fragment shader");
            }

            std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);

            shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[0].pNext = nullptr;

            shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStages[0].pName = "main";
            shaderStages[0].module = vshaderModule;

            shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[1].pNext = nullptr;

            shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderStages[1].pName = "main";
            shaderStages[1].module = fshaderModule;

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



            VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
            rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationInfo.pNext = nullptr;

            rasterizationInfo.depthClampEnable = VK_FALSE;
            rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
            //TODO: set this to make wireframes
            rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizationInfo.lineWidth = 1.0f;
            //TODO: set this to change cull mode
            rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
            rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
            //TODO: figure out what depth bias even is
            rasterizationInfo.depthBiasEnable = VK_FALSE;
            rasterizationInfo.depthBiasConstantFactor = 0.0f;
            rasterizationInfo.depthBiasClamp = 0.0f;
            rasterizationInfo.depthBiasSlopeFactor = 0.0f;


            VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
            multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleInfo.pNext = nullptr;

            multisampleInfo.sampleShadingEnable = VK_FALSE;
            //multisampling defaulted to no multisampling (1 sample per pixel)
            multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampleInfo.minSampleShading = 1.0f;
            multisampleInfo.pSampleMask = nullptr;
            multisampleInfo.alphaToCoverageEnable = VK_FALSE;
            multisampleInfo.alphaToOneEnable = VK_FALSE;


            //TODO: make this an option per shader
            VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

            //TODO: set blend state
            //setup dummy color blending. We aren't using transparent objects yet
            //the blending is just "no blend", but we do write to the color attachment
            VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {};
            colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendingInfo.pNext = nullptr;

            colorBlendingInfo.logicOpEnable = VK_FALSE;
            colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
            colorBlendingInfo.attachmentCount = 1;
            colorBlendingInfo.pAttachments = &colorBlendAttachment;
            colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;   // Optional
            colorBlendingInfo.blendConstants[0] = 0.0f;     // Optional
            colorBlendingInfo.blendConstants[1] = 0.0f;     // Optional
            colorBlendingInfo.blendConstants[2] = 0.0f;     // Optional
            colorBlendingInfo.blendConstants[3] = 0.0f;     // Optional



            VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
            depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

            depthStencilInfo.depthTestEnable = VK_TRUE;
            depthStencilInfo.depthWriteEnable = VK_TRUE;

            depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
            depthStencilInfo.minDepthBounds = 0.0f;  // Optional
            depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
            depthStencilInfo.stencilTestEnable = VK_FALSE;
            depthStencilInfo.front = {};  // Optional
            depthStencilInfo.back = {};   // Optional


            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.pNext = nullptr;

            std::vector<VkVertexInputAttributeDescription> attributes;
            VkVertexInputBindingDescription bindingDescription{};

            if(vertexDescription != nullptr)
            {
                for(auto& att : vertexDescription->attributes())
                {
                    attributes.push_back({.location = att.binding, .binding = 0, .format = VulkanTexture::formatFromCrossPlatform(att.storageType), .offset = att.offset});
                }
            }


            //TODO: add overwrites
            std::vector<VulkanUniformSet> overwrites;

            generateReflectionData(vertexCode,fragmentCode,attributes,bindingDescription,overwrites);
            vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
            vertexInputInfo.vertexAttributeDescriptionCount = attributes.size();
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.vertexBindingDescriptionCount = 1;

            std::vector<VkDescriptorSetLayout> setLayouts;
            for(int i=0; i< _uniformSets.size(); i++)
            {
                VkDescriptorSetLayout layout = _uniformSets[i].descriptorSetLayout();
                if(layout !=nullptr)
                {
                    setLayouts.push_back(layout);
                }
            }

            std::vector<VkPushConstantRange> setPushConstants;
            for(int i=0; i< _pushConstantRanges.size(); i++)
            {
                setPushConstants.push_back(_pushConstantRanges[i].range());
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.pNext = nullptr;
            //TODO: this is where we set the information from spir-v reflect I think
            pipelineLayoutInfo.flags = 0;
            pipelineLayoutInfo.setLayoutCount = setLayouts.size();
            pipelineLayoutInfo.pSetLayouts = setLayouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = setPushConstants.size();
            pipelineLayoutInfo.pPushConstantRanges = setPushConstants.data();


            if(vkCreatePipelineLayout(static_cast<VkDevice>(VulkanLib::graphicsCard()->device()),&pipelineLayoutInfo, nullptr,&_pipelineLayout)!= VK_SUCCESS)
            {
                throw std::runtime_error("Unable to create shader layout");
            }



            //TODO: most of the interesting stuff is in here.... I may need to enable more
            std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamicInfo{};
            dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicInfo.pDynamicStates = dynamicStates.data();


            VkGraphicsPipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

            pipelineInfo.stageCount = 2;
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

            auto colorAttachments =std::vector<VkFormat>(framebufferDescription.colorTargetCount());
            for(int i=0; i< framebufferDescription.colorTargetCount(); i++)
            {
                colorAttachments[i] = VulkanTexture::formatFromCrossPlatform(framebufferDescription.colorFormat(i));
            }

            VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                    .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
                    .pColorAttachmentFormats = colorAttachments.data(),
            };
            pipeline_rendering_create_info.depthAttachmentFormat = VulkanTexture::formatFromCrossPlatform(framebufferDescription.depthFormat());
            //TODO: stencil?

            pipelineInfo.pNext = &pipeline_rendering_create_info;

            //it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case

            auto result = vkCreateGraphicsPipelines(static_cast<VkDevice>(VulkanLib::graphicsCard()->device()), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);
            assert(result == VK_SUCCESS && "Unable to create shader");

            vkDestroyShaderModule(static_cast<VkDevice>(VulkanLib::graphicsCard()->device()),vshaderModule, nullptr);
            vkDestroyShaderModule(static_cast<VkDevice>(VulkanLib::graphicsCard()->device()),fshaderModule, nullptr);

            auto pipeline = _pipeline;
            auto pipelineLayout = _pipelineLayout;

            freeResources = [=]()
            {
                vkDestroyPipeline(VulkanLib::graphicsCard()->device(),static_cast<VkPipeline>(pipeline), nullptr);
                vkDestroyPipelineLayout(VulkanLib::graphicsCard()->device(),static_cast<VkPipelineLayout>(pipelineLayout),nullptr);
            };

        }

        void VulkanShader::generateReflectionData(const std::vector<char>& vertexCode, const std::vector<char>& fragmentCode, std::vector<VkVertexInputAttributeDescription>& attributes, VkVertexInputBindingDescription& binding, std::vector<VulkanUniformSet>& overwrites)
        {
            SpvReflectShaderModule vertexModule, fragmentModule;
            spvReflectCreateShaderModule(vertexCode.size(),vertexCode.data(),&vertexModule);
            spvReflectCreateShaderModule(fragmentCode.size(),fragmentCode.data(),&fragmentModule);
            try
            {
                std::vector<VulkanUniformSet> vertexGroups, fragmentGroups;
                uint32_t vertGroupCount = 0;
                auto result = spvReflectEnumerateDescriptorSets(&vertexModule,&vertGroupCount, nullptr);
                assert(result == SPV_REFLECT_RESULT_SUCCESS && "Unable to get reflection data");
                for(uint32_t i =0; i< vertGroupCount; i++)
                {
                    auto binding = vertexModule.descriptor_bindings[i];
                    auto set = spvReflectGetDescriptorSet(&vertexModule,binding.set,&result);
                    if(set!= nullptr)
                    {
                        vertexGroups.push_back(VulkanUniformSet(set, VK_SHADER_STAGE_VERTEX_BIT));
                    }
                }


                uint32_t fragGroupCount = 0;
                result = spvReflectEnumerateDescriptorSets(&fragmentModule,&fragGroupCount, nullptr);
                assert(result == SPV_REFLECT_RESULT_SUCCESS && "Unable to get reflection data");
                for(uint32_t i =0; i< fragGroupCount; i++)
                {
                    auto binding = fragmentModule.descriptor_bindings[i];
                    auto set = spvReflectGetDescriptorSet(&fragmentModule,binding.set,&result);
                    if(set != nullptr)
                    {
                        fragmentGroups.push_back(VulkanUniformSet(set, VK_SHADER_STAGE_FRAGMENT_BIT));
                    }
                }

                for(int i=0; i<4; i++)
                {
                    VulkanUniformSet fullGroup(i);
                    for(auto vertexIterator = vertexGroups.begin(); vertexIterator < vertexGroups.end(); vertexIterator++)
                    {
                        if(vertexIterator->index()==i)
                        {
                            fullGroup = std::move(*vertexIterator);
                        }
                    }

                    for(auto fragmentIterator = fragmentGroups.begin(); fragmentIterator < fragmentGroups.end(); fragmentIterator++)
                    {
                        if(fragmentIterator->index()==i)
                        {
                            fullGroup.merge(std::move(*fragmentIterator));
                        }
                    }

                    _uniformSets.push_back(std::move(fullGroup));
                }

                //emplace overwrites
                for(auto& overwrite : overwrites)
                {
                    _uniformSets[overwrite.index()] = std::move(overwrite);
                }

                //push constants
                uint32_t blockCount = 0;
                result = spvReflectEnumeratePushConstantBlocks(&vertexModule,&blockCount, nullptr);
                assert(result == SPV_REFLECT_RESULT_SUCCESS && "Unable to get push constant data");
                for(uint32_t i=0; i< blockCount; i++)
                {
                    auto& pushConstantRangeData = *spvReflectGetPushConstantBlock(&vertexModule,i,&result);
                    VkPushConstantRange range{};
                    range.size = pushConstantRangeData.size;
                    range.offset = pushConstantRangeData.offset;
                    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                    _pushConstantRanges.push_back(VulkanPushConstantRange(range));
                }

                result = spvReflectEnumeratePushConstantBlocks(&fragmentModule,&blockCount, nullptr);
                assert(result == SPV_REFLECT_RESULT_SUCCESS && "Unable to get push constant data");
                for(uint32_t i=0; i< blockCount; i++)
                {
                    auto& pushConstantRangeData = *spvReflectGetPushConstantBlock(&fragmentModule,i,&result);
                    VkPushConstantRange range{};
                    range.size = pushConstantRangeData.size;
                    range.offset = pushConstantRangeData.offset;
                    range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    _pushConstantRanges.push_back(range);
                }


                uint32_t count = 0;
                result = spvReflectEnumerateInputVariables(&vertexModule, &count, NULL);
                assert(result == SPV_REFLECT_RESULT_SUCCESS);


                //vertex attributes
                //only get attributes via reflection if they weren't supplied automatically
                if(attributes.empty())
                {

                    std::vector<SpvReflectInterfaceVariable *> input_vars(count);
                    result =
                            spvReflectEnumerateInputVariables(&vertexModule, &count, input_vars.data());
                    assert(result == SPV_REFLECT_RESULT_SUCCESS);

                    count = 0;
                    result = spvReflectEnumerateOutputVariables(&vertexModule, &count, NULL);
                    assert(result == SPV_REFLECT_RESULT_SUCCESS);

                    binding.binding = 0;
                    binding.stride = 0;  // computed below
                    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                    attributes.reserve(input_vars.size());
                    for (size_t i_var = 0; i_var < input_vars.size(); ++i_var)
                    {
                        const SpvReflectInterfaceVariable &refl_var = *(input_vars[i_var]);
                        // ignore built-in variables
                        if (refl_var.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
                        {
                            continue;
                        }
                        VkVertexInputAttributeDescription attr_desc{};
                        attr_desc.location = refl_var.location;
                        attr_desc.binding = binding.binding;
                        attr_desc.format = static_cast<VkFormat>(refl_var.format);
                        attr_desc.offset = 0;  // final offset computed below after sorting.
                        attributes.push_back(attr_desc);
                    }
                    // Sort attributes by location
                    std::sort(std::begin(attributes),
                              std::end(attributes),
                              [](const VkVertexInputAttributeDescription &a,
                                 const VkVertexInputAttributeDescription &b)
                              {
                                  return a.location < b.location;
                              });
                }

                // Compute final offsets of each attribute, and total vertex stride.
                for (auto &attribute: attributes)
                {
                    uint32_t format_size = VulkanTexture::formatSize(attribute.format) / 8;
                    attribute.offset = binding.stride;
                    binding.stride += format_size;
                }
            }
            catch (std::runtime_error err)
            {
                //cleanup
                spvReflectDestroyShaderModule(&vertexModule);
                spvReflectDestroyShaderModule(&fragmentModule);
                //throw
                throw err;
            }
            spvReflectDestroyShaderModule(&vertexModule);
            spvReflectDestroyShaderModule(&fragmentModule);
        }

        void *VulkanShader::GPUID()
        {
            return _pipeline;
        }

        UniformSet *VulkanShader::getUniformSet(size_t index)
        {
            return &_uniformSets[index];
        }

        PushConstantRange *VulkanShader::getPushConstantRange(size_t index)
        {
            return &_pushConstantRanges[index];
        }

        size_t VulkanShader::pushConstantRangeCount()
        {
            return _pushConstantRanges.size();
        }

        VkPipeline VulkanShader::pipeline()
        {
            return _pipeline;
        }

        VkPipelineLayout VulkanShader::layout()
        {
            return _pipelineLayout;
        }

        VulkanShader::VulkanShader(VulkanShader&& from): Resource(std::move(from))
        {
            move(std::move(from));
        }

        VulkanShader& VulkanShader::operator=(VulkanShader&& from)
        {
            Resource::operator=(std::move(from));
            move(std::move(from));
            return *this;
        }

        void VulkanShader::move(VulkanShader&& from)
        {
            std::swap(_pipelineLayout,from._pipelineLayout);
            std::swap(_pipeline, from._pipeline);
            _uniformSets.swap(from._uniformSets);
            _pushConstantRanges.swap(from._pushConstantRanges);
        }

    } // slag
} // vulkan