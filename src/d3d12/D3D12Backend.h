#pragma once

// D3D12 backend: entry points (one per line in BackendFunctions.inl) and the
// backend's object types.

#include "common/Backend.h"
#include "common/Objects.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// The Agility SDK's d3d12.h comes first on the include path (SlagDependencies.cmake).
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>

#include <atomic>
#include <cstdint>

namespace slag {

const BackendInfo* slagBackend_d3d12();

} // namespace slag

namespace slag::d3d12 {

#define SLAG_BACKEND_FN(ReturnType, name, parameters) ReturnType name parameters;
#include "common/BackendFunctions.inl"
#undef SLAG_BACKEND_FN

template <class T>
void release(T*& object)
{
    if (object)
    {
        object->Release();
        object = nullptr;
    }
}

struct InstanceImpl : InstanceCommon
{
    IDXGIFactory6* factory = nullptr;
    bool debugLayer = false;
};

// One shader-visible descriptor heap (ADR 0029).
struct DescriptorHeap
{
    ID3D12DescriptorHeap* heap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {};
    uint32_t increment = 0; // bytes per slot
    uint32_t slots = 0;
};

struct GraphicsDeviceImpl : GraphicsDeviceCommon
{
    IDXGIAdapter4* adapter = nullptr;

    // Valid while initialized.
    ID3D12Device10* device = nullptr;
    ID3D12InfoQueue1* infoQueue = nullptr;
    DWORD messageCookie = 0;
    ID3D12RootSignature* rootSignature = nullptr; // the fixed layout (ADR 0003)
    DescriptorHeap resourceHeap;
    DescriptorHeap samplerHeap;
};

struct CommandQueueImpl : CommandQueueCommon
{
    ID3D12CommandQueue* queue = nullptr;
    // For queueWaitIdle.
    ID3D12Fence* idleFence = nullptr;
    std::atomic<uint64_t> idleValue = 0;
    // Reused by submit, so submitting doesn't allocate after warm-up.
    Vector<ID3D12CommandList*> commandLists;
};

struct PipelineImpl : PipelineCommon
{
    ID3D12PipelineState* pipeline = nullptr;
};

struct CommandBufferImpl : CommandBufferCommon
{
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12GraphicsCommandList7* list = nullptr;
    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    // Reused by barrier.
    Vector<D3D12_GLOBAL_BARRIER> globalBarriers;
    Vector<D3D12_BUFFER_BARRIER> bufferBarriers;
};

struct BufferImpl : BufferCommon
{
    ID3D12Resource* resource = nullptr; // committed until the M2 sub-allocator
};

struct SemaphoreImpl : SemaphoreCommon
{
    ID3D12Fence* fence = nullptr;
};

inline InstanceImpl& instanceOf(const GraphicsDeviceImpl& device)
{
    return *static_cast<InstanceImpl*>(device.instance);
}

inline D3D12_COMMAND_LIST_TYPE listType(CommandQueueType type)
{
    switch (type)
    {
    case CommandQueueType::Graphics:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case CommandQueueType::Compute:
        return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    case CommandQueueType::Copy:
        return D3D12_COMMAND_LIST_TYPE_COPY;
    }
    return D3D12_COMMAND_LIST_TYPE_DIRECT;
}

// HRESULT to Status; marks the device lost on removal.
Status toStatus(GraphicsDeviceImpl& device, HRESULT result);

void setObjectName(ID3D12Object* object, const char* name);

// UTF-16 to UTF-8; empty if it doesn't fit.
void toUtf8(const wchar_t* text, char* out, size_t outSize);

} // namespace slag::d3d12
