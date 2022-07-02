#include "Shader.h"
#include <VkBootstrap.h>
#include <spirv_reflect.h>
#include <iostream>

namespace slag
{

    Shader::Shader(GraphicsCard* card, uint32_t *vertexCode, size_t vertexCodeSize, const char* vertexEntryPointName, uint32_t *fragmentCode, size_t fragmentCodeSize, const char* fragmentEntryPointName, std::shared_ptr<FrameBuffer> framebuffer)
    {
        _card = card;
        VkShaderModuleCreateInfo createVertexInfo = {};
        createVertexInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createVertexInfo.codeSize = vertexCodeSize* sizeof (uint32_t);
        createVertexInfo.pCode = vertexCode;

        VkShaderModule vshaderModule;
        if(vkCreateShaderModule(static_cast<VkDevice>(card->_vkDevice),&createVertexInfo, nullptr,&vshaderModule)!= VK_SUCCESS)
        {
            throw std::runtime_error("Unable to use vertex shader");
        }

        VkShaderModuleCreateInfo createFragmentInfo = {};
        createFragmentInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createFragmentInfo.codeSize = fragmentCodeSize* sizeof (uint32_t);
        createFragmentInfo.pCode = fragmentCode;

        VkShaderModule fshaderModule;
        if(vkCreateShaderModule(static_cast<VkDevice>(card->_vkDevice),&createFragmentInfo, nullptr,&fshaderModule)!= VK_SUCCESS)
        {
            throw std::runtime_error("Unable to use fragment shader");
        }

        VkPipelineShaderStageCreateInfo shaderStages[2]{};

        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].pNext = nullptr;

        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].pName = vertexEntryPointName;
        shaderStages[0].module = vshaderModule;

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].pNext = nullptr;

        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].pName = fragmentEntryPointName;
        shaderStages[1].module = fshaderModule;


        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.pNext = nullptr;

        //TODO: This will change when we add vertex info and attributes
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

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
        colorBlendingInfo.blendConstants[1] = 0.0f;     // Optional
        colorBlendingInfo.blendConstants[2] = 0.0f;     // Optional
        colorBlendingInfo.blendConstants[3] = 0.0f;     // Optional
        colorBlendingInfo.blendConstants[4] = 0.0f;     // Optional

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


        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pNext = nullptr;
        //TODO: this is where we set the information from spri-v reflect I think
        pipelineLayoutInfo.flags = 0;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;


        VkPipelineLayout pipelineLayout;
        if(vkCreatePipelineLayout(static_cast<VkDevice>(card->_vkDevice),&pipelineLayoutInfo, nullptr,&pipelineLayout)!= VK_SUCCESS)
        {
            throw std::runtime_error("Unable to create shader layout");
        }
        _vkPipelineLayout = pipelineLayout;



        //TODO: most of the interesting stuff is in here.... I may need to enable more
        std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicInfo{};
        dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicInfo.pDynamicStates = dynamicStates.data();


        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = nullptr;

        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &assemblyStateInfo;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizationInfo;
        pipelineInfo.pMultisampleState = &multisampleInfo;
        pipelineInfo.pColorBlendState = &colorBlendingInfo;
        pipelineInfo.pDepthStencilState = &depthStencilInfo;
        pipelineInfo.layout = pipelineLayout;
        //TODO: figure out if there's a way to not have to pass in the framebuffer for this information
        pipelineInfo.renderPass = static_cast<VkRenderPass>(framebuffer->_vkRenderPass);
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.pDynamicState = &dynamicInfo;

        //it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
        VkPipeline newPipeline;
        if (vkCreateGraphicsPipelines(static_cast<VkDevice>(card->_vkDevice), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Unable to create shader");
        }
        _vkPipeline = newPipeline;

        vkDestroyShaderModule(static_cast<VkDevice>(card->_vkDevice),vshaderModule, nullptr);
        vkDestroyShaderModule(static_cast<VkDevice>(card->_vkDevice),fshaderModule, nullptr);
    }

    Shader::~Shader()
    {
        destroy();
    }

    void Shader::destroy()
    {
        if(!_hasCleanedUp)
        {
            _hasCleanedUp = true;
            vkDestroyPipeline(static_cast<VkDevice>(_card->_vkDevice),static_cast<VkPipeline>(_vkPipeline), nullptr);
            vkDestroyPipelineLayout(static_cast<VkDevice>(_card->_vkDevice),static_cast<VkPipelineLayout>(_vkPipelineLayout),nullptr);
        }
    }
}