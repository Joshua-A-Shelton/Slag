#include "common/Logging.h"
#include "d3d12/D3D12Backend.h"

namespace slag::d3d12 {

namespace {

const ShaderBinary* findDxil(const ShaderStage& stage)
{
    for (const ShaderBinary& binary : stage.binaries)
    {
        if (binary.format == ShaderFormat::Dxil)
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
    const ShaderBinary* binary = findDxil(desc.compute);
    if (!binary || binary->size == 0)
    {
        logMessage(messageSink(device.instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createComputePipeline: no valid DXIL binary was offered (pipeline \"%s\")",
                   desc.debugName ? desc.debugName : "");
        return Status::Unsupported;
    }

    auto* pipeline = device.instance->allocator.create<PipelineImpl>();
    if (!pipeline)
    {
        return Status::OutOfHostMemory;
    }
    pipeline->device = &device;
    pipeline->functions = device.functions;

    // Every pipeline uses the device's root signature (ADR 0003).
    D3D12_COMPUTE_PIPELINE_STATE_DESC stateDesc = {};
    stateDesc.pRootSignature = device.rootSignature;
    stateDesc.CS.pShaderBytecode = binary->data;
    stateDesc.CS.BytecodeLength = binary->size;
    HRESULT result = device.device->CreateComputePipelineState(&stateDesc, IID_PPV_ARGS(&pipeline->pipeline));
    if (FAILED(result))
    {
        device.instance->allocator.destroy(pipeline);
        return toStatus(device, result);
    }
    setObjectName(pipeline->pipeline, desc.debugName);
    return static_cast<PipelineCommon*>(pipeline);
}

void destroyPipeline(GraphicsDeviceCommon* common, PipelineCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* pipeline = static_cast<PipelineImpl*>(object);
    release(pipeline->pipeline);
    device.instance->allocator.destroy(pipeline);
}

} // namespace slag::d3d12
