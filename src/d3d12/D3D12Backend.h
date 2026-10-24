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
#include <mutex>

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

// CPU-only RTV or DSV descriptors for attachment views (ADR 0035), handed out
// in groups (DSVs: one per read-only variant). Thread-safe.
struct CpuDescriptorPool
{
    D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    uint32_t groupSize = 1;
    uint32_t increment = 0;
    Vector<ID3D12DescriptorHeap*> heaps;
    Vector<SIZE_T> freeGroups;
    std::mutex lock;
};

struct GraphicsDeviceImpl : GraphicsDeviceCommon
{
    IDXGIAdapter4* adapter = nullptr;

    // Valid while initialized.
    ID3D12Device12* device = nullptr;
    ID3D12InfoQueue1* infoQueue = nullptr;
    DWORD messageCookie = 0;
    ID3D12RootSignature* rootSignature = nullptr; // the fixed layout (ADR 0003)
    DescriptorHeap resourceHeap;
    DescriptorHeap samplerHeap;
    CpuDescriptorPool rtvPool;
    CpuDescriptorPool dsvPool;
    // ExecuteIndirect signatures, indexed by IndirectKind (ADR 0047).
    ID3D12CommandSignature* commandSignatures[3] = {};
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
    D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED; // set at bind (ADR 0042)
};

struct CommandBufferImpl : CommandBufferCommon
{
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12GraphicsCommandList7* list = nullptr;
    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    // Reused by barrier.
    Vector<D3D12_GLOBAL_BARRIER> globalBarriers;
    Vector<D3D12_BUFFER_BARRIER> bufferBarriers;
    Vector<D3D12_TEXTURE_BARRIER> textureBarriers;
};

struct MemoryBlockImpl : MemoryBlockCommon
{
    ID3D12Heap* heap = nullptr;
};

struct BufferImpl : BufferCommon
{
    ID3D12Resource* resource = nullptr; // committed, or placed in a memory block
};

struct TextureImpl : TextureCommon
{
    ID3D12Resource* resource = nullptr; // committed, or placed in a memory block
    uint32_t planeCount = 1;            // 2 for depth/stencil formats
};

// The DSV group holds the variants in this order (ADR 0035).
enum DsvVariant : uint32_t
{
    DsvWritable = 0,
    DsvReadOnlyDepth = 1,
    DsvReadOnlyStencil = 2,
    DsvReadOnly = 3,
    DsvVariantCount = 4,
};

struct AttachmentViewImpl : AttachmentViewCommon
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = {}; // RTV, or the first DSV variant
    bool depthStencil = false;
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

void initPool(GraphicsDeviceImpl& device, CpuDescriptorPool& pool, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t groupSize);
void destroyPool(CpuDescriptorPool& pool);
// Ok with the group's first handle.
Status allocateGroup(GraphicsDeviceImpl& device, CpuDescriptorPool& pool, D3D12_CPU_DESCRIPTOR_HANDLE& handle);
void freeGroup(CpuDescriptorPool& pool, D3D12_CPU_DESCRIPTOR_HANDLE handle);

D3D12_COMPARISON_FUNC toComparisonFunc(CompareOp op);

// HRESULT to Status; marks the device lost on removal.
Status toStatus(GraphicsDeviceImpl& device, HRESULT result);

void setObjectName(ID3D12Object* object, const char* name);

// UTF-16 to UTF-8; empty if it doesn't fit.
void toUtf8(const wchar_t* text, char* out, size_t outSize);

} // namespace slag::d3d12
