#pragma once

// Opt-in D3D12 interop (ADR 0071): the native objects behind Slag's. Includes
// d3d12.h (from the Agility SDK) and dxgi1_6.h, so no other public header may
// include this one.
//
// The native objects stay Slag's: don't release them. A buffer or texture
// gets new ones when a defragmentation pass moves it (ADR 0062), swapchain
// images when the swapchain is reconfigured (ADR 0058).
//
// Native commands may be recorded into native(commandBuffer) between Slag
// calls. Slag then assumes only that its descriptor heaps and root signature
// (bound at begin()) are still bound; set everything else again through Slag.

#include <slag/Slag.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>
#include <dxgi1_6.h>

namespace slag::d3d12 {

// The device and the fixed pipeline layout every Slag pipeline uses (ADR 0003).
struct NativeDevice
{
    ID3D12Device12* device = nullptr; // null until initialized
    IDXGIAdapter4* adapter = nullptr;
    ID3D12DescriptorHeap* resourceHeap = nullptr; // shader-visible; slot i is descriptor i
    ID3D12DescriptorHeap* samplerHeap = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;
};

IDXGIFactory6* native(Instance* instance);
NativeDevice native(GraphicsDevice* device);
ID3D12CommandQueue* native(CommandQueue* queue);
ID3D12GraphicsCommandList7* native(CommandBuffer* commandBuffer);
ID3D12Heap* native(MemoryBlock* memoryBlock);
ID3D12Resource* native(Buffer* buffer);
ID3D12Resource* native(Texture* texture);
D3D12_CPU_DESCRIPTOR_HANDLE native(AttachmentView* view); // an RTV, or the writable DSV
ID3D12PipelineState* native(Pipeline* pipeline);
ID3D12QueryHeap* native(QueryPool* pool);
ID3D12Fence* native(Semaphore* semaphore);
IDXGISwapChain4* native(Swapchain* swapchain);

} // namespace slag::d3d12
