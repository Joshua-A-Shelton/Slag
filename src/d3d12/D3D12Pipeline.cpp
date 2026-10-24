#include "common/Logging.h"
#include "d3d12/D3D12Backend.h"
#include "d3d12/D3D12Formats.h"

#include <cstring>

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

namespace {

// One user (non-system-value) input of a DXIL input signature.
struct SignatureInput
{
    const char* semantic; // points into the shader binary
    uint32_t semanticIndex;
    uint32_t reg;
};

// Reads the ISG1 (or ISGN) part of a DXIL container (ADR 0038). Returns false
// when there is no input signature.
bool readInputSignature(const ShaderBinary& binary, Vector<SignatureInput>& inputs)
{
    auto* bytes = static_cast<const uint8_t*>(binary.data);
    auto read32 = [&](size_t offset) {
        uint32_t value = 0;
        if (offset + 4 <= binary.size)
        {
            std::memcpy(&value, bytes + offset, 4);
        }
        return value;
    };
    if (binary.size < 32 || std::memcmp(bytes, "DXBC", 4) != 0)
    {
        return false;
    }
    uint32_t partCount = read32(28);
    for (uint32_t part = 0; part < partCount; ++part)
    {
        uint32_t partOffset = read32(32 + part * 4);
        uint32_t partSize = read32(partOffset + 4);
        bool isg1 = std::memcmp(bytes + partOffset, "ISG1", 4) == 0;
        bool isgn = std::memcmp(bytes + partOffset, "ISGN", 4) == 0;
        if (!isg1 && !isgn)
        {
            continue;
        }
        size_t data = partOffset + 8;
        if (data + partSize > binary.size)
        {
            return false;
        }
        uint32_t count = read32(data);
        uint32_t elements = read32(data + 4);
        size_t stride = isg1 ? 32 : 24;
        for (uint32_t i = 0; i < count; ++i)
        {
            size_t element = data + elements + i * stride + (isg1 ? 4 : 0); // ISG1 starts with the stream
            uint32_t nameOffset = read32(element);
            uint32_t semanticIndex = read32(element + 4);
            uint32_t systemValue = read32(element + 8);
            uint32_t reg = read32(element + 16);
            if (systemValue != 0 || nameOffset >= partSize)
            {
                continue; // SV_VertexID and friends aren't vertex attributes
            }
            if (!inputs.pushBack({reinterpret_cast<const char*>(bytes + data + nameOffset), semanticIndex, reg}))
            {
                return false;
            }
        }
        // Location n is the n-th input by register.
        for (size_t i = 1; i < inputs.size(); ++i)
        {
            for (size_t j = i; j > 0 && inputs[j - 1].reg > inputs[j].reg; --j)
            {
                SignatureInput swap = inputs[j];
                inputs[j] = inputs[j - 1];
                inputs[j - 1] = swap;
            }
        }
        return true;
    }
    return false;
}

#ifdef _MSC_VER
#pragma warning(disable : 4324) // padded for alignas: intended, the stream needs it
#endif

template <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, class T>
struct alignas(void*) Subobject
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = Type;
    T value = {};
};

struct GraphicsStream
{
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*> rootSignature;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE> vertex;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE> pixel;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC> inputLayout;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE> topology;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2, D3D12_RASTERIZER_DESC2> rasterizer;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2, D3D12_DEPTH_STENCIL_DESC2> depthStencil;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC> blend;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY> targetFormats;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT> depthStencilFormat;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC> sample;
    Subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, UINT> sampleMask;
};

D3D12_BLEND toBlend(BlendFactor factor)
{
    switch (factor)
    {
    case BlendFactor::Zero:
        return D3D12_BLEND_ZERO;
    case BlendFactor::One:
        return D3D12_BLEND_ONE;
    case BlendFactor::SrcColor:
        return D3D12_BLEND_SRC_COLOR;
    case BlendFactor::InvSrcColor:
        return D3D12_BLEND_INV_SRC_COLOR;
    case BlendFactor::SrcAlpha:
        return D3D12_BLEND_SRC_ALPHA;
    case BlendFactor::InvSrcAlpha:
        return D3D12_BLEND_INV_SRC_ALPHA;
    case BlendFactor::SrcAlphaSaturate:
        return D3D12_BLEND_SRC_ALPHA_SAT;
    case BlendFactor::DestColor:
        return D3D12_BLEND_DEST_COLOR;
    case BlendFactor::InvDestColor:
        return D3D12_BLEND_INV_DEST_COLOR;
    case BlendFactor::DestAlpha:
        return D3D12_BLEND_DEST_ALPHA;
    case BlendFactor::InvDestAlpha:
        return D3D12_BLEND_INV_DEST_ALPHA;
    case BlendFactor::BlendConstant:
        return D3D12_BLEND_BLEND_FACTOR;
    case BlendFactor::InvBlendConstant:
        return D3D12_BLEND_INV_BLEND_FACTOR;
    case BlendFactor::Src1Color:
        return D3D12_BLEND_SRC1_COLOR;
    case BlendFactor::InvSrc1Color:
        return D3D12_BLEND_INV_SRC1_COLOR;
    case BlendFactor::Src1Alpha:
        return D3D12_BLEND_SRC1_ALPHA;
    case BlendFactor::InvSrc1Alpha:
        return D3D12_BLEND_INV_SRC1_ALPHA;
    }
    return D3D12_BLEND_ONE;
}

D3D12_DEPTH_STENCILOP_DESC1 toStencilFace(const StencilFaceState& face, const DepthStencilState& state)
{
    // Same order as D3D12, starting at 1.
    D3D12_DEPTH_STENCILOP_DESC1 result = {};
    result.StencilFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(face.failOp) + 1);
    result.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(face.depthFailOp) + 1);
    result.StencilPassOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(face.passOp) + 1);
    result.StencilFunc = toComparisonFunc(face.compareOp);
    result.StencilReadMask = state.stencilReadMask;
    result.StencilWriteMask = state.stencilWriteMask;
    return result;
}

void topologies(PrimitiveTopology topology, D3D12_PRIMITIVE_TOPOLOGY_TYPE& type, D3D_PRIMITIVE_TOPOLOGY& full)
{
    switch (topology)
    {
    case PrimitiveTopology::PointList:
        type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        full = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        return;
    case PrimitiveTopology::LineList:
        type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        full = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        return;
    case PrimitiveTopology::LineStrip:
        type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        full = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        return;
    case PrimitiveTopology::TriangleList:
        type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        full = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        return;
    case PrimitiveTopology::TriangleStrip:
        type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        full = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        return;
    }
}

} // namespace

Result<PipelineCommon*> createGraphicsPipeline(GraphicsDeviceCommon* common, const GraphicsPipelineDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    MessageSink sink = messageSink(device.instance);
    const char* name = desc.debugName ? desc.debugName : "";
    const ShaderBinary* vertex = findDxil(desc.vertex);
    const ShaderBinary* pixel = desc.pixel.binaries.empty() ? nullptr : findDxil(desc.pixel);
    if (!vertex || vertex->size == 0 || (!desc.pixel.binaries.empty() && (!pixel || pixel->size == 0)))
    {
        logMessage(sink, MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createGraphicsPipeline: no valid DXIL binary was offered (pipeline \"%s\")",
                   name);
        return Status::Unsupported;
    }

    // Vertex attributes by location, mapped to the shader's semantics (ADR 0038).
    Vector<SignatureInput> inputs(&device.instance->allocator);
    (void)readInputSignature(*vertex, inputs);
    D3D12_INPUT_ELEMENT_DESC elements[32] = {};
    UINT elementCount = 0;
    for (size_t location = 0; location < inputs.size() && location < 32; ++location)
    {
        const VertexAttribute* attribute = nullptr;
        for (const VertexAttribute& candidate : desc.vertexInput.attributes)
        {
            attribute = candidate.location == location ? &candidate : attribute;
        }
        if (!attribute)
        {
            logMessage(sink, MessageSeverity::Error, MessageSource::Slag,
                       "slag: GraphicsDevice::createGraphicsPipeline: the vertex shader reads location %zu (%s%u), "
                       "which has no vertex attribute (pipeline \"%s\")",
                       location, inputs[location].semantic, inputs[location].semanticIndex, name);
            return Status::Unsupported;
        }
        bool perInstance = attribute->slot < desc.vertexInput.slots.size() &&
                           desc.vertexInput.slots[attribute->slot] == VertexStepRate::PerInstance;
        D3D12_INPUT_ELEMENT_DESC& element = elements[elementCount++];
        element.SemanticName = inputs[location].semantic;
        element.SemanticIndex = inputs[location].semanticIndex;
        element.Format = toDxgiFormat(attribute->format);
        element.InputSlot = attribute->slot;
        element.AlignedByteOffset = attribute->offset;
        element.InputSlotClass =
            perInstance ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        element.InstanceDataStepRate = perInstance ? 1 : 0;
    }

    GraphicsStream stream;
    stream.rootSignature.value = device.rootSignature;
    stream.vertex.value = {vertex->data, vertex->size};
    if (pixel)
    {
        stream.pixel.value = {pixel->data, pixel->size};
    }
    stream.inputLayout.value = {elements, elementCount};
    D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    topologies(desc.topology, stream.topology.value, topology);

    const RasterizerState& raster = desc.rasterizer;
    D3D12_RASTERIZER_DESC2& rasterizer = stream.rasterizer.value;
    rasterizer.FillMode = raster.fillMode == FillMode::Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = raster.cullMode == CullMode::Front  ? D3D12_CULL_MODE_FRONT
                          : raster.cullMode == CullMode::Back ? D3D12_CULL_MODE_BACK
                                                              : D3D12_CULL_MODE_NONE;
    rasterizer.FrontCounterClockwise = raster.frontCounterClockwise;
    rasterizer.DepthBias = raster.depthBias;
    rasterizer.DepthBiasClamp = raster.depthBiasClamp;
    rasterizer.SlopeScaledDepthBias = raster.depthBiasSlopeScale;
    rasterizer.DepthClipEnable = raster.depthClip;
    rasterizer.LineRasterizationMode = D3D12_LINE_RASTERIZATION_MODE_ALIASED;
    rasterizer.ConservativeRaster = raster.conservativeRasterization ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON
                                                                     : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    const DepthStencilState& ds = desc.depthStencil;
    D3D12_DEPTH_STENCIL_DESC2& depthStencil = stream.depthStencil.value;
    depthStencil.DepthEnable = ds.depthTest;
    depthStencil.DepthWriteMask =
        ds.depthTest && ds.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStencil.DepthFunc = toComparisonFunc(ds.depthCompareOp);
    depthStencil.StencilEnable = ds.stencilTest;
    depthStencil.FrontFace = toStencilFace(ds.front, ds);
    depthStencil.BackFace = toStencilFace(ds.back, ds);
    depthStencil.DepthBoundsTestEnable = ds.depthBoundsTest;

    // Always independent per target (ADR 0041).
    D3D12_BLEND_DESC& blend = stream.blend.value;
    blend.AlphaToCoverageEnable = desc.blend.alphaToCoverage;
    blend.IndependentBlendEnable = TRUE;
    uint32_t colorCount = static_cast<uint32_t>(desc.colorFormats.size() < 8 ? desc.colorFormats.size() : 8);
    for (uint32_t i = 0; i < 8; ++i)
    {
        const ColorTargetBlend& target = desc.blend.targets[i];
        D3D12_RENDER_TARGET_BLEND_DESC& out = blend.RenderTarget[i];
        out.BlendEnable = target.blendEnable;
        out.SrcBlend = toBlend(target.srcColorFactor);
        out.DestBlend = toBlend(target.destColorFactor);
        out.BlendOp = static_cast<D3D12_BLEND_OP>(static_cast<int>(target.colorOp) + 1); // same order, from 1
        out.SrcBlendAlpha = toBlend(target.srcAlphaFactor);
        out.DestBlendAlpha = toBlend(target.destAlphaFactor);
        out.BlendOpAlpha = static_cast<D3D12_BLEND_OP>(static_cast<int>(target.alphaOp) + 1);
        out.LogicOp = D3D12_LOGIC_OP_NOOP;
        out.RenderTargetWriteMask = static_cast<UINT8>(target.writeMask); // same bits
    }
    stream.targetFormats.value.NumRenderTargets = colorCount;
    for (uint32_t i = 0; i < colorCount; ++i)
    {
        stream.targetFormats.value.RTFormats[i] = toDxgiFormat(desc.colorFormats[i]);
    }
    stream.depthStencilFormat.value = toDxgiFormat(desc.depthStencilFormat);
    stream.sample.value = {desc.sampleCount, 0};
    stream.sampleMask.value = desc.sampleMask;

    auto* pipeline = device.instance->allocator.create<PipelineImpl>();
    if (!pipeline)
    {
        return Status::OutOfHostMemory;
    }
    pipeline->device = &device;
    pipeline->functions = device.functions;
    pipeline->topology = topology;
    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {sizeof(stream), &stream};
    HRESULT result = device.device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipeline->pipeline));
    if (FAILED(result))
    {
        device.instance->allocator.destroy(pipeline);
        return toStatus(device, result);
    }
    setObjectName(pipeline->pipeline, desc.debugName);
    return static_cast<PipelineCommon*>(pipeline);
}

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
