#include "common/Logging.h"
#include "vulkan/VkBackend.h"

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
    VkShaderModuleCreateInfo moduleInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
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
    VkResult result =
        device.fn.vkCreateComputePipelines(device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline->pipeline);
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
