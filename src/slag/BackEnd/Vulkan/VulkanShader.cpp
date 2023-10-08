#include "VulkanShader.h"
#include "VulkanLib.h"
#include <stdexcept>
namespace slag
{
    namespace vulkan
    {
        VulkanShader::VulkanShader(const std::vector<char> &vertexCode, const std::vector<char> &fragmentCode, VertexDescription& vertexDescription)
        {
            VkShaderModuleCreateInfo createVertexInfo = {};
            createVertexInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createVertexInfo.codeSize = vertexCode.size();
            createVertexInfo.pCode = reinterpret_cast<const uint32_t*>(vertexCode.data());

            VkShaderModule vshaderModule;
            if(vkCreateShaderModule(VulkanLib::graphicsCard()->device(),&createVertexInfo, nullptr,&vshaderModule)!= VK_SUCCESS)
            {
                throw std::runtime_error("Unable to use vertex shader");
            }

            VkShaderModuleCreateInfo createFragmentInfo = {};
            createFragmentInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createFragmentInfo.codeSize = fragmentCode.size();
            createFragmentInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentCode.data());

            VkShaderModule fshaderModule;
            if(vkCreateShaderModule(VulkanLib::graphicsCard()->device(),&createFragmentInfo, nullptr,&fshaderModule)!= VK_SUCCESS)
            {
                throw std::runtime_error("Unable to use fragment shader");
            }

            VkPipelineShaderStageCreateInfo shaderStages[2]{};

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


            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.pNext = nullptr;

            _vertexDescription = VulkanVertexDescription(vertexDescription);
            //set vertex description
            vertexInputInfo.pVertexBindingDescriptions = _vertexDescription.bindings();
            vertexInputInfo.vertexBindingDescriptionCount = _vertexDescription.bindingsCount();
            vertexInputInfo.pVertexAttributeDescriptions = _vertexDescription.attributes();
            vertexInputInfo.vertexAttributeDescriptionCount = _vertexDescription.attributeCount();

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

            //TODO: set blend state
            VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

            //TODO: set up real transparency
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

            generateReflectionData(vertexCode,fragmentCode,std::move(overwrites));

            std::vector<VkDescriptorSetLayout> setLayouts;
            for(int i=0; i< _uniformGroups.size(); i++)
            {
                VkDescriptorSetLayout layout = _uniformGroups[i].descriptorSetLayout();
                if(layout !=nullptr)
                {
                    setLayouts.push_back(layout);
                }
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.pNext = nullptr;
            //TODO: this is where we set the information from spir-v reflect I think
            pipelineLayoutInfo.flags = 0;
            pipelineLayoutInfo.setLayoutCount = setLayouts.size();
            pipelineLayoutInfo.pSetLayouts = setLayouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = _pushConstantRanges.size();
            pipelineLayoutInfo.pPushConstantRanges = _pushConstantRanges.data();


            if(vkCreatePipelineLayout(static_cast<VkDevice>(SlagLib::graphicsCard()->device()),&pipelineLayoutInfo, nullptr,&_vkPipelineLayout)!= VK_SUCCESS)
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
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &assemblyStateInfo;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizationInfo;
            pipelineInfo.pMultisampleState = &multisampleInfo;
            pipelineInfo.pColorBlendState = &colorBlendingInfo;
            pipelineInfo.pDepthStencilState = &depthStencilInfo;
            pipelineInfo.layout = _vkPipelineLayout;
            pipelineInfo.renderPass = VK_NULL_HANDLE;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.pDynamicState = &dynamicInfo;

            auto colorAttachments =std::vector<VkFormat>( _framebufferDescription.colorAttachmentFormats().size());
            for(int i=0; i< _framebufferDescription.colorAttachmentFormats().size(); i++)
            {
                colorAttachments[i] = _framebufferDescription.colorAttachmentFormats()[i];
            }

            VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                    .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
                    .pColorAttachmentFormats = colorAttachments.data(),
            };
            pipeline_rendering_create_info.depthAttachmentFormat = _framebufferDescription.depthFormat();
            //TODO: stencil?

            pipelineInfo.pNext = &pipeline_rendering_create_info;

            //it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case

            auto result = vkCreateGraphicsPipelines(static_cast<VkDevice>(VulkanLib::graphicsCard()->device()), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);
            assert(result == VK_SUCCESS && "Unable to create shader");

            vkDestroyShaderModule(static_cast<VkDevice>(VulkanLib::graphicsCard()->device()),vshaderModule, nullptr);
            vkDestroyShaderModule(static_cast<VkDevice>(VulkanLib::graphicsCard()->device()),fshaderModule, nullptr);

        }
    } // slag
} // vulkan