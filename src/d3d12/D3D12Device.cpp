#include "common/Logging.h"
#include "d3d12/D3D12Backend.h"

#include <slag/Limits.h>

namespace slag::d3d12 {

namespace {

void __stdcall debugMessage(D3D12_MESSAGE_CATEGORY, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID,
                            LPCSTR description, void* context)
{
    MessageSeverity mapped = MessageSeverity::Info;
    if (severity <= D3D12_MESSAGE_SEVERITY_ERROR)
    {
        mapped = MessageSeverity::Error;
    }
    else if (severity == D3D12_MESSAGE_SEVERITY_WARNING)
    {
        mapped = MessageSeverity::Warning;
    }
    logMessage(messageSink(static_cast<const InstanceCommon*>(context)), mapped, MessageSource::Backend, "%s",
               description ? description : "");
}

HRESULT createHeap(GraphicsDeviceImpl& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t slots, const char* name,
                   DescriptorHeap& heap)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = type;
    desc.NumDescriptors = slots;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    HRESULT result = device.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap.heap));
    if (FAILED(result))
    {
        return result;
    }
    heap.cpuStart = heap.heap->GetCPUDescriptorHandleForHeapStart();
    heap.increment = device.device->GetDescriptorHandleIncrementSize(type);
    heap.slots = slots;
    setObjectName(heap.heap, name);
    return S_OK;
}

// The fixed layout (ADR 0003): heap-indexed descriptors and 128 bytes of root
// constants at b0, where Slang puts [[vk::push_constant]] blocks for DXIL.
HRESULT createRootSignature(GraphicsDeviceImpl& device)
{
    D3D12_ROOT_PARAMETER1 pushConstants = {};
    pushConstants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    pushConstants.Constants.ShaderRegister = 0;
    pushConstants.Constants.RegisterSpace = 0;
    pushConstants.Constants.Num32BitValues = limits::maxPushConstantsSize / 4;
    pushConstants.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
    desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    desc.Desc_1_1.NumParameters = 1;
    desc.Desc_1_1.pParameters = &pushConstants;
    desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
                          D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED |
                          D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* blob = nullptr;
    ID3DBlob* errors = nullptr;
    HRESULT result = D3D12SerializeVersionedRootSignature(&desc, &blob, &errors);
    if (FAILED(result))
    {
        logMessage(messageSink(device.instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::initialize: root signature serialization failed: %s",
                   errors ? static_cast<const char*>(errors->GetBufferPointer()) : "");
        release(errors);
        return result;
    }
    result = device.device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                                IID_PPV_ARGS(&device.rootSignature));
    release(blob);
    release(errors);
    if (SUCCEEDED(result))
    {
        setObjectName(device.rootSignature, "slag root signature");
    }
    return result;
}

void destroyQueues(GraphicsDeviceImpl& device)
{
    for (Vector<CommandQueue*>& queues : device.queues)
    {
        for (CommandQueue* object : queues)
        {
            auto* queue = static_cast<CommandQueueImpl*>(object);
            release(queue->idleFence);
            release(queue->queue);
            device.instance->allocator.destroy(queue);
        }
        queues.clear();
    }
}

} // namespace

Status toStatus(GraphicsDeviceImpl& device, HRESULT result)
{
    if (SUCCEEDED(result))
    {
        return Status::Ok;
    }
    switch (result)
    {
    case DXGI_ERROR_DEVICE_REMOVED:
    case DXGI_ERROR_DEVICE_HUNG:
    case DXGI_ERROR_DEVICE_RESET:
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
        return reportDeviceLost(&device);
    case E_OUTOFMEMORY:
        return Status::OutOfDeviceMemory;
    default:
        return Status::Unsupported;
    }
}

void setObjectName(ID3D12Object* object, const char* name)
{
    if (!object || !name)
    {
        return;
    }
    wchar_t wide[limits::maxDebugNameLength + 1];
    if (MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, static_cast<int>(limits::maxDebugNameLength + 1)) > 0)
    {
        object->SetName(wide);
    }
}

Result<void> initializeDevice(GraphicsDeviceCommon* common, const GraphicsDeviceInitDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    InstanceImpl& instance = instanceOf(device);

    HRESULT result = D3D12CreateDevice(device.adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device.device));
    if (FAILED(result))
    {
        logMessage(messageSink(&instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::initialize: D3D12CreateDevice failed (0x%08lx)",
                   static_cast<unsigned long>(result));
        device.device = nullptr;
        return toStatus(device, result);
    }

    // Debug layer messages go to the debug callback (ADR 0011).
    if (instance.debugLayer && SUCCEEDED(device.device->QueryInterface(IID_PPV_ARGS(&device.infoQueue))))
    {
        device.infoQueue->RegisterMessageCallback(debugMessage, D3D12_MESSAGE_CALLBACK_FLAG_NONE,
                                                  static_cast<InstanceCommon*>(&instance), &device.messageCookie);
    }

    // Queues: every Slag queue is its own D3D12 queue (ADR 0009).
    const uint32_t requested[3] = {desc.graphicsQueueCount, desc.computeQueueCount, desc.copyQueueCount};
    for (size_t type = 0; type < 3; ++type)
    {
        for (uint32_t i = 0; i < requested[type]; ++i)
        {
            auto* queue = instance.allocator.create<CommandQueueImpl>();
            if (!queue || !device.queues[type].pushBack(queue))
            {
                instance.allocator.destroy(queue);
                shutdownDevice(&device);
                return Status::OutOfHostMemory;
            }
            queue->device = &device;
            queue->functions = device.functions;
            queue->type = static_cast<CommandQueueType>(type);
            queue->commandLists = Vector<ID3D12CommandList*>(&instance.allocator);
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = listType(queue->type);
            result = device.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue->queue));
            if (SUCCEEDED(result))
            {
                result = device.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&queue->idleFence));
            }
            if (FAILED(result))
            {
                Status status = toStatus(device, result);
                shutdownDevice(&device);
                return status;
            }
        }
    }

    // Descriptor heaps and the fixed root signature (ADRs 0003, 0029).
    result = createHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, desc.resourceHeapSize, "slag resource heap",
                        device.resourceHeap);
    if (SUCCEEDED(result))
    {
        result = createHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, desc.samplerHeapSize, "slag sampler heap",
                            device.samplerHeap);
    }
    if (SUCCEEDED(result))
    {
        result = createRootSignature(device);
    }
    if (FAILED(result))
    {
        Status status = toStatus(device, result);
        shutdownDevice(&device);
        return status;
    }
    return {};
}

void shutdownDevice(GraphicsDeviceCommon* common)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    if (!device.device)
    {
        return;
    }
    for (Vector<CommandQueue*>& queues : device.queues)
    {
        for (CommandQueue* object : queues)
        {
            auto* queue = static_cast<CommandQueueImpl*>(object);
            if (queue->queue && queue->idleFence) // may be partly created when initialize failed
            {
                (void)queueWaitIdle(queue);
            }
        }
    }
    destroyQueues(device);
    release(device.rootSignature);
    release(device.resourceHeap.heap);
    release(device.samplerHeap.heap);
    device.resourceHeap = {};
    device.samplerHeap = {};
    if (device.infoQueue)
    {
        device.infoQueue->UnregisterMessageCallback(device.messageCookie);
        release(device.infoQueue);
    }
    release(device.device);
    device.initialized = false;
}

Result<void> deviceWaitIdle(GraphicsDeviceCommon* common)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    for (Vector<CommandQueue*>& queues : device.queues)
    {
        for (CommandQueue* queue : queues)
        {
            Result<void> result = queueWaitIdle(static_cast<CommandQueueImpl*>(queue));
            if (!result)
            {
                return result;
            }
        }
    }
    return {};
}

Result<void> queueWaitIdle(CommandQueueCommon* common)
{
    auto* queue = static_cast<CommandQueueImpl*>(common);
    auto& device = *static_cast<GraphicsDeviceImpl*>(queue->device);
    uint64_t value = ++queue->idleValue;
    HRESULT result = queue->queue->Signal(queue->idleFence, value);
    if (SUCCEEDED(result))
    {
        // A null event blocks until the fence reaches the value; a lost device
        // completes every fence (UINT64_MAX).
        result = queue->idleFence->SetEventOnCompletion(value, nullptr);
    }
    if (SUCCEEDED(result))
    {
        result = device.device->GetDeviceRemovedReason();
    }
    return toStatus(device, result);
}

} // namespace slag::d3d12
