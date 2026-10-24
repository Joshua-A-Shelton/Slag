# 0071 — Native interop accessors

**Status:** Accepted (2026-10-24, M8). Names are final (docs/naming.md;
settled after M8).

## Decision

v1.0 ships the accessors CLAUDE.md §6 promises; importing native objects
stays deferred (ADRs 0018, 0027).

- Opt-in headers: `slag/native/D3D12.h` (includes `d3d12.h` and `dxgi1_6.h`)
  and `slag/native/Vulkan.h` (includes `vulkan/vulkan.h`). Nothing else
  includes them. Using them puts the Agility SDK or Vulkan-Headers include
  directory on the user's path, which the `slag` target exports.
- One overload of `native(object)` per object type in `slag::d3d12` and
  `slag::vk`, returning the handle, or a small struct when there are
  several (a Vulkan buffer's `VkBuffer`, `VkDeviceMemory`, and offset).
  Calling one backend's accessor on another backend's object is a
  validation error and returns empty handles.
- Vulkan also hands out `vkGetInstanceProcAddr`, since Slag loads Vulkan
  itself (no `vulkan-1` link) and users need it to load their own entry
  points.
- The native objects stay Slag's: don't destroy or rename them, and expect
  them to change on defragmentation moves (ADR 0062) and swapchain
  reconfigure (ADR 0058).

## Native recording mid-stream

Native commands may be recorded into `native(commandBuffer)` between Slag
calls. Slag assumes afterwards only that what it set at `begin()` is still
bound:

- **D3D12:** the resource and sampler descriptor heaps (`SetDescriptorHeaps`)
  and the root signature on both graphics and compute.
- **Vulkan:** the resource and sampler descriptor heaps
  (`vkCmdBindResourceHeapEXT`, `vkCmdBindSamplerHeapEXT`).

Anything else (pipeline, push constants, constant buffer, vertex and index
buffers, viewports, scissors, blend constant, stencil reference, depth
bounds) must be set again through Slag after native commands that change
it. Render passes are opened and closed by one side only, Slag or native.

## Mapping

| Object | D3D12 | Vulkan |
|---|---|---|
| Instance | `IDXGIFactory6*` | `VkInstance` |
| GraphicsDevice | device, adapter, both descriptor heaps, root signature | instance, physical device, device |
| CommandQueue | `ID3D12CommandQueue*` | `VkQueue`, family index |
| CommandBuffer | `ID3D12GraphicsCommandList7*` | `VkCommandBuffer` |
| MemoryBlock | `ID3D12Heap*` | `VkDeviceMemory` |
| Buffer | `ID3D12Resource*` | `VkBuffer`, memory, offset |
| Texture | `ID3D12Resource*` | `VkImage`, memory, offset |
| AttachmentView | `D3D12_CPU_DESCRIPTOR_HANDLE` (RTV or DSV) | `VkImageView` |
| Pipeline | `ID3D12PipelineState*` | `VkPipeline` |
| QueryPool | `ID3D12QueryHeap*` | `VkQueryPool` |
| Semaphore | `ID3D12Fence*` | `VkSemaphore` (timeline) |
| Swapchain | `IDXGISwapChain4*` | `VkSwapchainKHR`, `VkSurfaceKHR` |
