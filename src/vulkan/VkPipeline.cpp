#include "common/Logging.h"
#include "vulkan/VkBackend.h"
#include "vulkan/VkFormats.h"

#include <cstdint>
#include <cstring>

namespace slag::vk {

namespace {

const ShaderBinary* findSpirv(const ShaderStage& stage)
{
    for (const ShaderBinary& binary : stage.binaries)
    {
        if (binary.format == ShaderFormat::Spirv)
        {
            return &binary;
        }
    }
    return nullptr;
}

} // namespace

namespace {

const VkDescriptorSetAndBindingMappingEXT constantBufferBinding = [] {
    VkDescriptorSetAndBindingMappingEXT mapping = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT};
    mapping.descriptorSet = 0;
    mapping.firstBinding = 1; // register b1
    mapping.bindingCount = 1;
    mapping.resourceMask = VK_SPIRV_RESOURCE_TYPE_UNIFORM_BUFFER_BIT_EXT;
    mapping.source = VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_ADDRESS_EXT;
    mapping.sourceData.pushAddressOffset = constantBufferPushOffset;
    return mapping;
}();

} // namespace

VkShaderDescriptorSetAndBindingMappingInfoEXT constantBufferMapping()
{
    VkShaderDescriptorSetAndBindingMappingInfoEXT info = {
        VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT};
    info.mappingCount = 1;
    info.pMappings = &constantBufferBinding;
    return info;
}

namespace {

// SPIR-V must be 4-byte aligned; copies into `storage` if the caller's data isn't.
const uint32_t* alignedCode(const ShaderBinary& binary, Vector<uint32_t>& storage, Status& status)
{
    status = Status::Ok;
    if (reinterpret_cast<uintptr_t>(binary.data) % alignof(uint32_t) == 0)
    {
        return static_cast<const uint32_t*>(binary.data);
    }
    size_t words = binary.size / 4;
    if (!storage.reserve(words))
    {
        status = Status::OutOfHostMemory;
        return nullptr;
    }
    for (size_t i = 0; i < words; ++i)
    {
        (void)storage.pushBack(0);
    }
    std::memcpy(storage.data(), binary.data, binary.size);
    return storage.data();
}

VkBlendFactor toBlendFactor(BlendFactor factor)
{
    switch (factor)
    {
    case BlendFactor::Zero:
        return VK_BLEND_FACTOR_ZERO;
    case BlendFactor::One:
        return VK_BLEND_FACTOR_ONE;
    case BlendFactor::SrcColor:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendFactor::InvSrcColor:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case BlendFactor::SrcAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendFactor::InvSrcAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::SrcAlphaSaturate:
        return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case BlendFactor::DestColor:
        return VK_BLEND_FACTOR_DST_COLOR;
    case BlendFactor::InvDestColor:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case BlendFactor::DestAlpha:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendFactor::InvDestAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case BlendFactor::BlendConstant:
        return VK_BLEND_FACTOR_CONSTANT_COLOR; // D3D12's blend factor is a full float4
    case BlendFactor::InvBlendConstant:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case BlendFactor::Src1Color:
        return VK_BLEND_FACTOR_SRC1_COLOR;
    case BlendFactor::InvSrc1Color:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
    case BlendFactor::Src1Alpha:
        return VK_BLEND_FACTOR_SRC1_ALPHA;
    case BlendFactor::InvSrc1Alpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }
    return VK_BLEND_FACTOR_ONE;
}

VkBlendOp toBlendOp(BlendOp op)
{
    switch (op)
    {
    case BlendOp::Add:
        return VK_BLEND_OP_ADD;
    case BlendOp::Subtract:
        return VK_BLEND_OP_SUBTRACT;
    case BlendOp::ReverseSubtract:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BlendOp::Min:
        return VK_BLEND_OP_MIN;
    case BlendOp::Max:
        return VK_BLEND_OP_MAX;
    }
    return VK_BLEND_OP_ADD;
}

VkPrimitiveTopology toTopology(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::PointList:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case PrimitiveTopology::LineList:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case PrimitiveTopology::LineStrip:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case PrimitiveTopology::TriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case PrimitiveTopology::TriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

VkStencilOpState toStencil(const StencilFaceState& face, const DepthStencilState& state)
{
    static_assert(static_cast<int>(StencilOp::DecrementWrap) == VK_STENCIL_OP_DECREMENT_AND_WRAP); // same order
    VkStencilOpState result = {};
    result.failOp = static_cast<VkStencilOp>(face.failOp);
    result.passOp = static_cast<VkStencilOp>(face.passOp);
    result.depthFailOp = static_cast<VkStencilOp>(face.depthFailOp);
    result.compareOp = toCompareOp(face.compareOp);
    result.compareMask = state.stencilReadMask;
    result.writeMask = state.stencilWriteMask;
    return result; // reference: set while recording (ADR 0037)
}

} // namespace

Result<PipelineCommon*> createGraphicsPipeline(GraphicsDeviceCommon* common, const GraphicsPipelineDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    const ShaderBinary* vertex = findSpirv(desc.vertex);
    const ShaderBinary* pixel = desc.pixel.binaries.empty() ? nullptr : findSpirv(desc.pixel);
    if (!vertex || vertex->size % 4 != 0 || (!desc.pixel.binaries.empty() && (!pixel || pixel->size % 4 != 0)))
    {
        logMessage(messageSink(device.instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createGraphicsPipeline: no valid SPIR-V binary was offered (pipeline \"%s\")",
                   desc.debugName ? desc.debugName : "");
        return Status::Unsupported;
    }

    // Shader code inline, no modules (ADR 0036).
    Vector<uint32_t> vertexStorage(&device.instance->allocator);
    Vector<uint32_t> pixelStorage(&device.instance->allocator);
    Status status = Status::Ok;
    VkShaderModuleCreateInfo modules[2] = {{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO},
                                           {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO}};
    VkPipelineShaderStageCreateInfo stages[2] = {{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
                                                 {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};
    VkShaderDescriptorSetAndBindingMappingInfoEXT mappings[2] = {constantBufferMapping(), constantBufferMapping()};
    modules[0].pNext = &mappings[0];
    modules[0].codeSize = vertex->size;
    modules[0].pCode = alignedCode(*vertex, vertexStorage, status);
    stages[0].pNext = &modules[0];
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].pName = desc.vertex.entryPoint;
    uint32_t stageCount = 1;
    if (pixel && status == Status::Ok)
    {
        modules[1].pNext = &mappings[1];
        modules[1].codeSize = pixel->size;
        modules[1].pCode = alignedCode(*pixel, pixelStorage, status);
        stages[1].pNext = &modules[1];
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].pName = desc.pixel.entryPoint;
        stageCount = 2;
    }
    if (status != Status::Ok)
    {
        return status;
    }

    // Vertex input by location (ADR 0038); strides are dynamic.
    VkVertexInputBindingDescription bindings[32] = {};
    VkVertexInputAttributeDescription attributes[32] = {};
    uint32_t bindingCount =
        static_cast<uint32_t>(desc.vertexInput.slots.size() < 32 ? desc.vertexInput.slots.size() : 32);
    uint32_t attributeCount =
        static_cast<uint32_t>(desc.vertexInput.attributes.size() < 32 ? desc.vertexInput.attributes.size() : 32);
    for (uint32_t i = 0; i < bindingCount; ++i)
    {
        bindings[i].binding = i;
        bindings[i].inputRate = desc.vertexInput.slots[i] == VertexStepRate::PerInstance ? VK_VERTEX_INPUT_RATE_INSTANCE
                                                                                         : VK_VERTEX_INPUT_RATE_VERTEX;
    }
    for (uint32_t i = 0; i < attributeCount; ++i)
    {
        const VertexAttribute& attribute = desc.vertexInput.attributes[i];
        attributes[i] = {attribute.location, attribute.slot, toVkFormat(attribute.format), attribute.offset};
    }
    VkPipelineVertexInputStateCreateInfo vertexInput = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = bindingCount;
    vertexInput.pVertexBindingDescriptions = bindings;
    vertexInput.vertexAttributeDescriptionCount = attributeCount;
    vertexInput.pVertexAttributeDescriptions = attributes;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = toTopology(desc.topology);
    VkPipelineViewportStateCreateInfo viewport = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};

    const RasterizerState& raster = desc.rasterizer;
    VkPipelineRasterizationStateCreateInfo rasterization = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization.depthClampEnable = raster.depthClip ? VK_FALSE : VK_TRUE;
    rasterization.polygonMode = raster.fillMode == FillMode::Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterization.cullMode = raster.cullMode == CullMode::Front  ? VK_CULL_MODE_FRONT_BIT
                             : raster.cullMode == CullMode::Back ? VK_CULL_MODE_BACK_BIT
                                                                 : VK_CULL_MODE_NONE;
    rasterization.frontFace = raster.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    rasterization.depthBiasEnable = raster.depthBias != 0.0f || raster.depthBiasSlopeScale != 0.0f ? VK_TRUE : VK_FALSE;
    rasterization.depthBiasConstantFactor = raster.depthBias;
    rasterization.depthBiasClamp = raster.depthBiasClamp;
    rasterization.depthBiasSlopeFactor = raster.depthBiasSlopeScale;
    rasterization.lineWidth = 1.0f;
    VkPipelineRasterizationConservativeStateCreateInfoEXT conservative = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT};
    if (raster.conservativeRasterization)
    {
        conservative.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
        rasterization.pNext = &conservative;
    }

    VkPipelineMultisampleStateCreateInfo multisample = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = static_cast<VkSampleCountFlagBits>(desc.sampleCount);
    VkSampleMask sampleMask = desc.sampleMask;
    multisample.pSampleMask = &sampleMask;
    multisample.alphaToCoverageEnable = desc.blend.alphaToCoverage ? VK_TRUE : VK_FALSE;

    const DepthStencilState& ds = desc.depthStencil;
    VkPipelineDepthStencilStateCreateInfo depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencil.depthTestEnable = ds.depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = ds.depthTest && ds.depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = toCompareOp(ds.depthCompareOp);
    depthStencil.depthBoundsTestEnable = ds.depthBoundsTest ? VK_TRUE : VK_FALSE;
    depthStencil.stencilTestEnable = ds.stencilTest ? VK_TRUE : VK_FALSE;
    depthStencil.front = toStencil(ds.front, ds);
    depthStencil.back = toStencil(ds.back, ds);
    depthStencil.maxDepthBounds = 1.0f;

    uint32_t colorCount = static_cast<uint32_t>(desc.colorFormats.size() < 8 ? desc.colorFormats.size() : 8);
    VkFormat colorFormats[8] = {};
    VkPipelineColorBlendAttachmentState blends[8] = {};
    for (uint32_t i = 0; i < colorCount; ++i)
    {
        const ColorTargetBlend& target = desc.blend.targets[i];
        colorFormats[i] = toVkFormat(desc.colorFormats[i]);
        blends[i].blendEnable = target.blendEnable ? VK_TRUE : VK_FALSE;
        blends[i].srcColorBlendFactor = toBlendFactor(target.srcColorFactor);
        blends[i].dstColorBlendFactor = toBlendFactor(target.destColorFactor);
        blends[i].colorBlendOp = toBlendOp(target.colorOp);
        blends[i].srcAlphaBlendFactor = toBlendFactor(target.srcAlphaFactor);
        blends[i].dstAlphaBlendFactor = toBlendFactor(target.destAlphaFactor);
        blends[i].alphaBlendOp = toBlendOp(target.alphaOp);
        blends[i].colorWriteMask = static_cast<VkColorComponentFlags>(target.writeMask); // same bits
    }
    VkPipelineColorBlendStateCreateInfo colorBlend = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlend.attachmentCount = colorCount;
    colorBlend.pAttachments = blends;

    // Set while recording (ADR 0037). Strides only with vertex buffers: the
    // state must be set before every draw otherwise.
    VkDynamicState dynamicStates[6] = {VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT, VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
                                       VK_DYNAMIC_STATE_BLEND_CONSTANTS, VK_DYNAMIC_STATE_STENCIL_REFERENCE};
    uint32_t dynamicCount = 4;
    if (bindingCount > 0)
    {
        dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE;
    }
    if (ds.depthBoundsTest)
    {
        dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_DEPTH_BOUNDS;
    }
    VkPipelineDynamicStateCreateInfo dynamic = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = dynamicCount;
    dynamic.pDynamicStates = dynamicStates;

    // Dynamic rendering: attachment formats instead of a render pass (ADR 0042).
    VkPipelineRenderingCreateInfo rendering = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    rendering.colorAttachmentCount = colorCount;
    rendering.pColorAttachmentFormats = colorFormats;
    if (desc.depthStencilFormat != PixelFormat::Undefined)
    {
        rendering.depthAttachmentFormat = toVkFormat(desc.depthStencilFormat);
        if (hasTraits(desc.depthStencilFormat, FormatTraits::Stencil))
        {
            rendering.stencilAttachmentFormat = rendering.depthAttachmentFormat;
        }
    }
    // Descriptor heaps replace pipeline layouts (ADR 0003).
    VkPipelineCreateFlags2CreateInfo flags = {VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO};
    flags.pNext = &rendering;
    flags.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT;

    VkGraphicsPipelineCreateInfo createInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    createInfo.pNext = &flags;
    createInfo.stageCount = stageCount;
    createInfo.pStages = stages;
    createInfo.pVertexInputState = &vertexInput;
    createInfo.pInputAssemblyState = &inputAssembly;
    createInfo.pViewportState = &viewport;
    createInfo.pRasterizationState = &rasterization;
    createInfo.pMultisampleState = &multisample;
    createInfo.pDepthStencilState = &depthStencil;
    createInfo.pColorBlendState = &colorBlend;
    createInfo.pDynamicState = &dynamic;
    createInfo.layout = VK_NULL_HANDLE;

    auto* pipeline = device.instance->allocator.create<PipelineImpl>();
    if (!pipeline)
    {
        return Status::OutOfHostMemory;
    }
    pipeline->device = &device;
    pipeline->functions = device.functions;
    pipeline->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    VkResult result = device.fn.vkCreateGraphicsPipelines(device.device, device.pipelineCache, 1, &createInfo, nullptr,
                                                          &pipeline->pipeline);
    if (result != VK_SUCCESS)
    {
        device.instance->allocator.destroy(pipeline);
        return toStatus(device, result);
    }
    setObjectName(device, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(pipeline->pipeline), desc.debugName);
    return static_cast<PipelineCommon*>(pipeline);
}

Result<PipelineCommon*> createComputePipeline(GraphicsDeviceCommon* common, const ComputePipelineDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    const ShaderBinary* binary = findSpirv(desc.compute);
    if (!binary || binary->size == 0 || binary->size % 4 != 0)
    {
        logMessage(messageSink(device.instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createComputePipeline: no valid SPIR-V binary was offered (pipeline \"%s\")",
                   desc.debugName ? desc.debugName : "");
        return Status::Unsupported;
    }

    // SPIR-V must be 4-byte aligned; copy if the caller's data isn't.
    Vector<uint32_t> aligned(&device.instance->allocator);
    const uint32_t* code = static_cast<const uint32_t*>(binary->data);
    if (reinterpret_cast<uintptr_t>(binary->data) % alignof(uint32_t) != 0)
    {
        size_t words = binary->size / 4;
        if (!aligned.reserve(words))
        {
            return Status::OutOfHostMemory;
        }
        for (size_t i = 0; i < words; ++i)
        {
            (void)aligned.pushBack(0);
        }
        std::memcpy(aligned.data(), binary->data, binary->size);
        code = aligned.data();
    }

    // No shader module: the code is passed inline (Vulkan 1.4, ADR 0036).
    VkShaderDescriptorSetAndBindingMappingInfoEXT mapping = constantBufferMapping();
    VkShaderModuleCreateInfo moduleInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    moduleInfo.pNext = &mapping;
    moduleInfo.codeSize = binary->size;
    moduleInfo.pCode = code;
    VkPipelineShaderStageCreateInfo stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.pNext = &moduleInfo;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.pName = desc.compute.entryPoint;

    // Descriptor heaps replace pipeline layouts (ADR 0003).
    VkPipelineCreateFlags2CreateInfo flags = {VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO};
    flags.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT;
    VkComputePipelineCreateInfo createInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    createInfo.pNext = &flags;
    createInfo.stage = stage;
    createInfo.layout = VK_NULL_HANDLE;

    auto* pipeline = device.instance->allocator.create<PipelineImpl>();
    if (!pipeline)
    {
        return Status::OutOfHostMemory;
    }
    pipeline->device = &device;
    pipeline->functions = device.functions;
    pipeline->bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    VkResult result = device.fn.vkCreateComputePipelines(device.device, device.pipelineCache, 1, &createInfo, nullptr,
                                                         &pipeline->pipeline);
    if (result != VK_SUCCESS)
    {
        device.instance->allocator.destroy(pipeline);
        return toStatus(device, result);
    }
    setObjectName(device, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(pipeline->pipeline), desc.debugName);
    return static_cast<PipelineCommon*>(pipeline);
}

void destroyPipeline(GraphicsDeviceCommon* common, PipelineCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* pipeline = static_cast<PipelineImpl*>(object);
    device.fn.vkDestroyPipeline(device.device, pipeline->pipeline, nullptr);
    device.instance->allocator.destroy(pipeline);
}

} // namespace slag::vk
