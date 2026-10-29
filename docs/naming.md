# Naming

Final names chosen by the project owner. Anything not listed here is still
a placeholder (CLAUDE.md §13).

## Object types

| Name | What it is | ADR |
|---|---|---|
| `GraphicsDevice` | One GPU: listed by the instance, then initialized | 0008 |
| `MemoryBlock` | Explicit memory that placed resources live in | 0023 |
| `QueryPool` | Container of queries | 0051 |
| `AttachmentView` | Render-target / depth-stencil view | 0035 |
| `Instance` | Backend instance that lists graphics devices | 0008 |
| `CommandQueue` | Graphics, compute, or copy queue | 0009, 0054 |
| `CommandBuffer` | Recorded commands, owning its allocator | 0045 |
| `Swapchain` | Window presentation | 0056 |
| `Buffer` | GPU buffer | 0020 |
| `Texture` | Any of the seven texture types | 0021 |
| `Pipeline` | Graphics or compute pipeline | 0037 |
| `Semaphore` | Timeline semaphore — Slag's only sync primitive (no binary semaphores are exposed) | 0053 |
| `PixelFormat` | Format enum for textures, views, and vertex attributes | 0022, 0038 |

Older notes may say "queue", "command list", "fence", or "format" in prose;
they mean the types above. Names not listed here are still placeholders.

## Conventions

- **Creation structs** end in `Desc` (`BufferDesc`, `Texture2DDesc`).
  "Descriptor" is reserved for GPU descriptors.
- **Create functions** are `GraphicsDevice` methods returning `Result<T*>`:
  `Result<Buffer*> r = device->createBuffer(desc);`
- **Destroy** is one overloaded `destroy()` on the device:
  `device->destroy(buffer);`
- **Texture types** follow shader spelling: `Texture2D`, `Texture2DArray`,
  `TextureCube` (`createTexture2D`, `Texture2DDesc`).
- **Queries have no `get` prefix:** `device->info()`, `semaphore->value()`.
- **Structs nested in a `Desc`** don't end in `Desc`: pipeline state structs
  end in `State`, the rest are plain nouns.

- **Writable descriptor kinds** use HLSL's `RW` prefix:
  `writeRWStructuredBuffer`, `writeRWTexture`.
- **Indirect calls with a GPU count** are separate `...Count` variants:
  `drawIndirect` / `drawIndirectCount`.
- **Command buffer state:** `bind*` for objects and buffers (`bindPipeline`,
  `bindIndexBuffer`), `set*` for plain values (`setViewports`).
- **Constants:** `pushConstants` and `setConstantBuffer`.

## Instance and devices

| Name | Notes |
|---|---|
| `slag::createInstance(InstanceDesc)` → `Result<Instance*>` | Backend tag, common options, backend-options union |
| `slag::destroy(Instance*)` | Free-function overload |
| `slag::isBackendAvailable(Backend)` → `bool` | Whether a backend is compiled in (ADR 0065) |
| `instance->graphicsDevices()` | Span of `GraphicsDevice*` |
| `device->info()` → `GraphicsDeviceInfo` | Name, vendor, memory, LUID |
| `device->capabilities()` → `GraphicsDeviceCapabilities` | Optional features, per-device values |
| `device->initialize(GraphicsDeviceInitDesc)` → `Result<void>` | Heap sizes, cache blob, queue counts |
| `device->shutdown()` | Undoes `initialize` (recovery after device loss) |
| `device->commandQueue(CommandQueueType, index)` → `CommandQueue*` | Queues are fetched, not created |

## Create functions

| Function | Struct |
|---|---|
| `createBuffer` | `BufferDesc` |
| `createTexture1D` / `createTexture1DArray` / `createTexture2D` / `createTexture2DArray` / `createTexture3D` / `createTextureCube` / `createTextureCubeArray` | `Texture1DDesc` / `Texture1DArrayDesc` / `Texture2DDesc` / `Texture2DArrayDesc` / `Texture3DDesc` / `TextureCubeDesc` / `TextureCubeArrayDesc` |
| `createAttachmentView` | `AttachmentViewDesc` |
| `createMemoryBlock` | `MemoryBlockDesc` |
| `createGraphicsPipeline` / `createComputePipeline` | `GraphicsPipelineDesc` / `ComputePipelineDesc` |
| `createQueryPool` | `QueryPoolDesc` |
| `createSemaphore` | `SemaphoreDesc` (initial value) |
| `createCommandBuffer` | `CommandBufferDesc` (queue type) |
| `createSwapchain` | `SwapchainDesc` |
| `swapchain->reconfigure` | `SwapchainReconfigureDesc` (size, image count, format, present mode, color space) |

Every `Desc` has an optional `debugName` and `userData` (`uint64_t`); every
object has `userData()` / `setUserData()` (ADR 0063).

## Device queries

| Name | Returns |
|---|---|
| `device->placementInfo(desc)` (overloaded per creation struct) | `PlacementInfo { size, alignment }` |
| `device->copyFootprint(texture, mip)` | Buffer size and row pitch for one mip |
| `device->supportedSwapchainFormats(NativeWindow)` | Format + color space pairs |

## Command buffer methods

| Area | Methods |
|---|---|
| Lifetime | `begin()`, `end()`, `reset()` |
| Render passes | `beginRendering(RenderingDesc)`, `endRendering()` (suspend/resume are flags in `RenderingDesc`) |
| Pipeline and buffers | `bindPipeline`, `bindVertexBuffers(firstSlot, span<VertexBufferBinding>)`, `bindIndexBuffer(buffer, offset, size, IndexFormat)` |
| Values | `setViewports(span<Viewport>)`, `setScissors(span<Rect>)`, `setBlendConstant`, `setStencilReference`, `setDepthBounds(min, max)` |
| Constants | `pushConstants(data, size, offset)`, `setConstantBuffer(address, size)` |
| Draws | `draw`, `drawIndexed`, `drawIndirect`, `drawIndirectCount`, `drawIndexedIndirect`, `drawIndexedIndirectCount` |
| Dispatches | `dispatch(x, y, z)`, `dispatchIndirect(buffer, offset)` |
| Barriers | `barrier(span<GlobalBarrier>, span<BufferBarrier>, span<TextureBarrier>)` |
| Copies | `copyBuffer`, `copyTexture`, `copyBufferToTexture`, `copyTextureToBuffer`, `resolveTexture` |
| Queries | `writeTimestamp(pool, index)`, `beginQuery`, `endQuery`, `resolveQueries(pool, first, count, buffer, offset)` |
| Labels | `beginLabel(name, color)`, `endLabel()`, `insertLabel(name, color)` |

## Queue methods

`submit(SubmitDesc)`, `present(swapchain, PresentMode)`, `waitIdle()`,
`timestampFrequency()`.

## Device methods

| Area | Methods |
|---|---|
| Resource descriptors | `writeConstantBuffer`, `writeRawBuffer`, `writeStructuredBuffer`, `writeTypedBuffer`, `writeRWRawBuffer`, `writeRWStructuredBuffer`, `writeRWTypedBuffer`, `writeTexture`, `writeRWTexture` |
| Sampler descriptors | `writeSampler` |
| Clearing slots | `clearResourceDescriptor(index)`, `clearSamplerDescriptor(index)` |
| Waiting | `waitIdle()` |
| State and info | `status()`, `memoryBudget()` → `MemoryBudget`, `pipelineCacheData()`, `formatSupport(PixelFormat)` → `FormatSupport` |

- Every `write*` takes `(index, …Descriptor)` and has a batch overload taking
  a span of `{index, …Descriptor}`.
- Write structs end in `Descriptor` (`StructuredBufferDescriptor`,
  `TextureDescriptor`, `SamplerDescriptor`): they describe GPU descriptors,
  the one thing the word is reserved for.

## Other object methods

| Object | Methods |
|---|---|
| `Swapchain` | `acquire()` → `Result<uint32_t>` (ADR 0060), `texture(index)`, `attachmentView(index)`, `srgbAttachmentView(index)`, `imageCount()`, `switchablePresentModes()`, `reconfigure(desc)`, `setHdrMetadata(HdrMetadata)`, `setMaxFrameLatency(n)`, `waitForNextFrame(timeoutNs)` |
| `Semaphore` | `value()`, `signal(value)`, `wait(value, timeoutNs)`; the free function `slag::waitSemaphores(span<SemaphoreValue>, WaitMode, timeoutNs)` waits on several (moved off the device after M8) |
| `Buffer` | `mappedData()`, `gpuAddress()` |
| `SemaphoreValue` | `{ Semaphore* semaphore; uint64_t value; }` |

## Fields and small names (settled during M1)

| Where | Names |
|---|---|
| `InstanceDesc` | `backend`, `validationFlags`, `validationFailureAction`, `messageCallback`, `messageUserData`, `allocatorCallbacks`, `devicePreference`, `allowSoftwareDevices`, `crashDiagnostics`, `applicationName`, `applicationVersion`, `engineName`, `engineVersion`, `backendOptions`, `debugName`, `userData` |
| Backend options | union `BackendOptions` with members `d3d12` (`D3D12Options`) and `vulkan` (`VulkanOptions`) |
| `ValidationFlags` | adds `None` |
| `GraphicsDeviceInfo` | `name`, `driverVersion`, `vendorId`, `deviceId`, `deviceType`, `dedicatedMemoryBytes`, `sharedMemoryBytes`, `luid` |
| `GraphicsDeviceInitDesc` | `resourceHeapSize`, `samplerHeapSize`, `pipelineCacheData`, `graphicsQueueCount`, `computeQueueCount`, `copyQueueCount` |
| Small additions | `CommandQueue::type()`, `SemaphoreDesc::initialValue`, `BufferDesc::size`, `CommandBufferDesc::queueType`, `ComputePipelineDesc::compute` |
| Placement | `PlacementOptions` (`movable`); `Placement::automatic(kind, options)`, `Placement::placed(memoryBlock, offset)`; fields `kind`, `movable`, `memoryBlock`, `offset` |
| Shaders | `ShaderBinary` (`format`, `data`, `size`); `ShaderStage` (`binaries`, `entryPoint`, default `"main"`) |
| Descriptors | `StructuredBufferDescriptor` (`buffer`, `offset`, `count`, `stride`) |
| `SubresourceRange` | `baseMip`, `mipCount`, `baseLayer`, `layerCount`, `plane`; counts default to the static constants `SubresourceRange::remainingMips` / `remainingLayers` |

New names added while implementing go here once confirmed.

## Fields and small names (settled after M2)

| Where | Names |
|---|---|
| `PixelFormat` | adds `Undefined` (the default in every `Desc`) |
| `FormatSupport` (flags) | `None`, `Sampled`, `LinearFilter`, `RenderTarget`, `Blend`, `Multisample2`, `Multisample4`, `Multisample8`, `ShaderWrite`, `ShaderWriteLoad`, `Atomics`, `DepthStencil`, `VertexBuffer`, `TypedBuffer`, `RWTypedBuffer` (shader-write names match `TextureUsage::ShaderWrite`) |
| `GraphicsDeviceCapabilities` | `unifiedMemory`, `gpuUploadMemory`, `maxBufferSize` |
| `MemoryBudget` | `dedicated`, `shared`, each a `MemoryUsage` (`budgetBytes`, `usageBytes`) |
| `MemoryBlockDesc` | `size`, `kind`, `debugName`, `userData`; `MemoryBlock::size()`, `kind()` |
| `TextureUsage` | adds `None` |
| Texture `Desc`s | `format`, `width`, `height`, `depth`, `layerCount`, `size` (cube faces), `cubeCount`, `mipCount`, `sampleCount`, `usage`, `castableFormats`, `clearValue` (pointer; null means none), `placement`, `debugName`, `userData` |
| `ClearValue` | `color[4]`, `depth`, `stencil` |
| `AttachmentViewDesc` | `texture`, `format` (`Undefined`: the texture's), `mip`, `baseLayer`, `layerCount`, `debugName`, `userData` |
| Copies | `TextureLocation` (`texture`, `mip`, `layer`, `x`, `y`, `z`, `plane`), `BufferLocation` (`buffer`, `offset`, `rowPitch`); `copyTexture(destination, source, width, height, depth)`, `copyBufferToTexture(destination, source, …)`, `copyTextureToBuffer(destination, source, …)` |
| Footprint | `copyFootprint(texture, mip, plane = Color)` → `CopyFootprint` (`size`, `rowPitch`, `rowCount`, `width`, `height`, `depth`) |
| Semantics | `SubresourceRange::plane = Color` covers every plane of a depth/stencil texture |

## Fields and small names (settled after M3)

| Where | Names |
|---|---|
| Buffer descriptors | `ConstantBufferDescriptor` (`address`, `size`), `RawBufferDescriptor` (`buffer`, `offset`, `size`), `TypedBufferDescriptor` (`buffer`, `offset`, `count`, `format`) |
| Texture descriptors | `TextureDescriptor` (`texture`, `viewType`, `format`, `baseMip`, `mipCount`, `baseLayer`, `layerCount`, `plane`), `RWTextureDescriptor` (`texture`, `viewType`, `format`, `mip`, `baseLayer`, `layerCount`) |
| `SamplerDescriptor` | `minFilter`, `magFilter`, `mipFilter`, `addressU`, `addressV`, `addressW`, `mipLodBias`, `maxAnisotropy`, `compareEnable`, `compareOp`, `minLod`, `maxLod`, `borderColor`, `reductionMode` |
| Batch writes | `…DescriptorWrite` (`index`, `descriptor`) per kind: `ConstantBufferDescriptorWrite`, `RawBufferDescriptorWrite`, `StructuredBufferDescriptorWrite`, `TypedBufferDescriptorWrite`, `TextureDescriptorWrite`, `RWTextureDescriptorWrite`, `SamplerDescriptorWrite` |
| Capabilities | `samplerReduction`, `conservativeRasterization`, `depthBounds` |
| Vertex input | `VertexAttribute` (`location`, `format`, `slot`, `offset`; `location` replaces `semantic`/`semanticIndex`), `VertexInputLayout` (`attributes`, `slots`) |
| `RasterizerState` | `fillMode`, `cullMode`, `frontCounterClockwise`, `depthBias`, `depthBiasClamp`, `depthBiasSlopeScale`, `depthClip`, `conservativeRasterization` |
| Depth/stencil | `StencilFaceState` (`failOp`, `depthFailOp`, `passOp`, `compareOp`), `DepthStencilState` (`depthTest`, `depthWrite`, `depthCompareOp`, `stencilTest`, `front`, `back`, `stencilReadMask`, `stencilWriteMask`, `depthBoundsTest`) |
| Blend | `ColorTargetBlend` (`blendEnable`, `srcColorFactor`, `destColorFactor`, `colorOp`, `srcAlphaFactor`, `destAlphaFactor`, `alphaOp`, `writeMask`), `BlendState` (`alphaToCoverage`, `targets[8]`); `ColorWriteMask` adds `None` |
| `GraphicsPipelineDesc` | `vertex`, `pixel`, `vertexInput`, `topology`, `rasterizer`, `depthStencil`, `blend`, `colorFormats`, `depthStencilFormat`, `sampleCount`, `sampleMask`, `debugName`, `userData` |
| Render passes | `ColorAttachment` (`view`, `loadOp`, `storeOp`, `clearValue`; `resolveView` cut in M7), `DepthStencilAttachment` (`view`, `depthLoadOp`, `depthStoreOp`, `stencilLoadOp`, `stencilStoreOp`, `clearValue`, `readOnlyDepth`, `readOnlyStencil`), `RenderingDesc` (`colorAttachments`, `depthStencilAttachment`, `suspending`, `resuming`) |
| Small types | `Viewport` (`x`, `y`, `width`, `height`, `minDepth`, `maxDepth`), `Rect` (`x`, `y`, `width`, `height`), `VertexBufferBinding` (`buffer`, `offset`, `size`, `stride`) |
| Indirect arguments | `DrawArguments` (`vertexCount`, `instanceCount`, `firstVertex`, `firstInstance`), `DrawIndexedArguments` (`indexCount`, `instanceCount`, `firstIndex`, `vertexOffset`, `firstInstance`), `DispatchArguments` (`groupCountX`, `groupCountY`, `groupCountZ`) |
| Command parameters | `drawIndirect(argumentBuffer, argumentOffset, drawCount)`, `drawIndirectCount(argumentBuffer, argumentOffset, countBuffer, countOffset, maxDrawCount)` (and the indexed pair), `dispatchIndirect(argumentBuffer, argumentOffset)`, `resolveTexture(destination, destinationMip, destinationLayer, source, sourceLayer)` |
| Behavior | `beginRendering` starts with a full-area viewport and scissor (`StoreOp::Resolve` cut in M7, ADR 0049) |

## Fields and small names (settled after M4)

| Where | Names |
|---|---|
| `NativeWindow` | `kind`, `display`, `window` (shape from ADR 0055) |
| `SwapchainDesc` | `window`, `queue`, `width`, `height`, `minImageCount`, `format`, `colorSpace`, `presentMode`, `debugName`, `userData` |
| `SwapchainReconfigureDesc` | `width`, `height`, `minImageCount`, `format`, `colorSpace`, `presentMode` |
| `SwapchainFormat` | `format`, `colorSpace` |
| `HdrMetadata` | `redPrimary[2]`, `greenPrimary[2]`, `bluePrimary[2]`, `whitePoint[2]`, `maxMasteringLuminance`, `minMasteringLuminance`, `maxContentLightLevel`, `maxFrameAverageLightLevel` |
| Device queries | `supportedSwapchainFormats(window, span<SwapchainFormat>)` → count, and new `supportedPresentModes(window, span<PresentMode>)` → count (fill the span, return how many exist) |
| `Swapchain` | `attachmentView(index)` and `srgbAttachmentView(index)` (8-bit formats) |
| Capabilities | `frameLatency` |

## Fields and small names (settled after M5)

| Where | Names |
|---|---|
| `QueryPoolDesc` | `type`, `count`, `queueType` (Graphics and Compute pools are interchangeable; Copy pools only on copy queues), `debugName`, `userData` |
| `CommandQueue` | `calibrateTimestamps()` → `Result<TimestampCalibration>` |
| `TimestampCalibration` | `gpuTimestamp` (writeTimestamp ticks), `cpuNanoseconds` |
| Capabilities | `timestampValidBits`, `copyQueueTimestamps`, `timestampCalibration` |
| `QueryPool` | `userData()`, `setUserData(value)` |
| `DescriptorAllocator` | `DescriptorAllocator(device, DescriptorAllocatorDesc)`, `allocate()` → slot or 0, `release(slot, SemaphoreValue)` (not `free`: MSVC's `_CRTDBG_MAP_ALLOC` macro), `collect()` |
| `DescriptorAllocatorDesc` | `heap`, `firstSlot`, `slotCount` |
| `DescriptorHeap` | `Resource`, `Sampler` (utility-only for now; moves to the core with the same meaning if heaps are exposed) |
| `DeferredDestroyer` | `destroyAfter` overloads for `AttachmentView`, `Buffer`, `Texture`, `QueryPool`, `Pipeline`, `CommandBuffer`, `MemoryBlock`, `Semaphore` |

## Fields and small names (settled after M8)

| Where | Names |
|---|---|
| Native interop (ADR 0071) | `slag::d3d12::native(object)`, `slag::vk::native(object)`, one overload per object type |
| D3D12 structs | `NativeDevice { device, adapter, resourceHeap, samplerHeap, rootSignature }` |
| Vulkan structs | `NativeDevice { instance, physicalDevice, device }`, `NativeQueue { queue, family }`, `NativeBuffer { buffer, memory, offset }`, `NativeTexture { image, memory, offset }`, `NativeSwapchain { swapchain, surface }` |
| Vulkan | `slag::vk::getInstanceProcAddr(instance)` |

## Fields and small names (settled after M7)

| Where | Names |
|---|---|
| `CommandBuffer` labels | `beginLabel(name, const float* color = nullptr)`, `insertLabel(name, color)` (color: optional RGBA pointer) |
| `CommandBuffer` occlusion queries | `beginQuery(pool, index)`, `endQuery(pool, index)` (the names were planned; the index on `endQuery` is new) |
| Sample | `samples/crash` |

## Fields and small names (settled after M6)

| Where | Names |
|---|---|
| File | `slag/Defragmentation.h` |
| `GraphicsDevice` | `beginDefragmentation(DefragmentationDesc)` → `Result<DefragmentationPass*>`, `endDefragmentation(pass, SemaphoreValue completed)` → `Result<void>` |
| `DefragmentationDesc` | `kind`, `maxBytes`, `maxMoves` |
| `DefragmentationMove` | `buffer`, `texture` (one set) |
| `DefragmentationPass` | `moves()`, `recordCopies(commandBuffer, layouts)` |
| `PlacementOptions` | `movable` (from ADR 0062, never reviewed) |

## Nested structs

`AllocatorCallbacks` (in `InstanceDesc`; ADR 0066).

## Utilities (`slag/util/`)

| Name | What it is | ADR |
|---|---|---|
| `DeferredDestroyer` | `destroyAfter(object, SemaphoreValue)`, `collect()`, `flush()` | 0069 |
| `DescriptorAllocator` | `allocate()`, `release(slot, SemaphoreValue)`, `collect()` | 0032 |

`RasterizerState`, `DepthStencilState`, `BlendState`, `ColorTargetBlend`,
`VertexInputLayout`, `VertexAttribute`, `ShaderStage`, `ShaderBinary`,
`Placement`, `ClearValue`, `NativeWindow`.

## Result and status

`Result<T>`, `Status` (`Ok`, `Suboptimal`, `Occluded`, `Timeout`;
errors `OutOfDate`, `DeviceLost`, `OutOfDeviceMemory`, `OutOfHostMemory`,
`Unsupported`) — ADR 0060.

## Enums

Flag enums use a singular name with `SLAG_FLAGS`.

| Enum | Values |
|---|---|
| `Backend` | `D3D12`, `Vulkan` (0x80+ reserved for external backends) |
| `ValidationFlags` (flags) | `Api`, `GpuAssisted`, `Synchronization` |
| `ValidationFailureAction` | `Break`, `Continue`, `Abort` (ADR 0067) |
| `MessageSeverity` | `Error`, `Warning`, `Info` |
| `MessageSource` | `Slag`, `Backend` |
| `GraphicsDeviceType` | `Discrete`, `Integrated`, `Software` |
| `DevicePreference` | `HighPerformance`, `LowPower` |
| `CommandQueueType` | `Graphics`, `Compute`, `Copy` |
| `WaitMode` | `All`, `Any` |
| `MemoryKind` | `GpuOnly`, `Upload`, `Readback`, `GpuUpload` |
| `TextureUsage` (flags) | `RenderTarget`, `DepthStencil`, `ShaderWrite` |
| `TexturePlane` | `Color`, `Depth`, `Stencil` |
| `TextureViewType` | `Texture1D`, `Texture1DArray`, `Texture2D`, `Texture2DArray`, `Texture2DMS`, `Texture2DMSArray`, `Texture3D`, `TextureCube`, `TextureCubeArray` |
| `PixelFormat` | ADR 0022 and 0038 |
| `IndexFormat` | `Uint16`, `Uint32` |
| `QueryType` | `Timestamp`, `Occlusion` |
| `ShaderFormat` | `Dxil`, `Spirv` (0x80+ reserved for external backends) |
| `VertexStepRate` | `PerVertex`, `PerInstance` |
| `PrimitiveTopology` | `PointList`, `LineList`, `LineStrip`, `TriangleList`, `TriangleStrip` |
| `FillMode` | `Solid`, `Wireframe` |
| `CullMode` | `None`, `Front`, `Back` |
| `CompareOp` | `Never`, `Less`, `Equal`, `LessEqual`, `Greater`, `NotEqual`, `GreaterEqual`, `Always` |
| `StencilOp` | `Keep`, `Zero`, `Replace`, `IncrementSaturate`, `DecrementSaturate`, `Invert`, `IncrementWrap`, `DecrementWrap` |
| `BlendFactor` | `Zero`, `One`, `SrcColor`, `InvSrcColor`, `SrcAlpha`, `InvSrcAlpha`, `SrcAlphaSaturate`, `DestColor`, `InvDestColor`, `DestAlpha`, `InvDestAlpha`, `BlendConstant`, `InvBlendConstant`, `Src1Color`, `InvSrc1Color`, `Src1Alpha`, `InvSrc1Alpha` |
| `BlendOp` | `Add`, `Subtract`, `ReverseSubtract`, `Min`, `Max` |
| `ColorWriteMask` (flags) | `Red`, `Green`, `Blue`, `Alpha`, `All` |
| `Filter` | `Point`, `Linear` |
| `AddressMode` | `Wrap`, `Mirror`, `Clamp`, `Border`, `MirrorOnce` |
| `BorderColor` | `TransparentBlack`, `OpaqueBlack`, `OpaqueWhite` |
| `ReductionMode` | `Standard`, `Minimum`, `Maximum` |
| `LoadOp` | `Load`, `Clear`, `Discard` |
| `StoreOp` | `Store`, `Discard`, `Resolve` |
| `BarrierSync` (flags) | values from ADR 0049 |
| `BarrierAccess` (flags) | values from ADR 0049 |
| `TextureLayout` | values from ADR 0049 |
| `PresentMode` | `Fifo`, `Mailbox`, `Immediate` |
| `ColorSpace` | `SrgbNonlinear`, `Hdr10`, `ScRgbLinear` |
| `WindowKind` | `Win32`, `Xlib`, `Xcb`, `Wayland` |

Sample count is a plain `uint32_t` (1, 2, 4, 8), not an enum.

## Constants (`slag/Limits.h`)

Grouped by purpose in lowercase namespaces inside `slag` (`limits`, `alignment`, `defaults`), with camelCase names. A constant tied to one type is a `static constexpr` member of it (`SubresourceRange::remainingMips`). Internal constants are plain camelCase.

| Name | Value | ADR |
|---|---|---|
| `limits::maxTexture1DSize` | 16384 | 0028 |
| `limits::maxTexture2DSize` | 16384 | 0028 |
| `limits::maxTexture3DSize` | 2048 | 0028 |
| `limits::maxTextureCubeSize` | 16384 | 0028 |
| `limits::maxTextureArrayLayers` | 2048 | 0028 |
| `limits::maxMipLevels` | 15 | 0028 |
| `limits::maxSampleCount` | 8 | 0021 |
| `limits::maxConstantBufferSize` | 65536 | 0028 |
| `alignment::constantBuffer` | 256 | 0020 |
| `alignment::rawBuffer` | 256 | 0030 |
| `limits::maxPushConstantsSize` | 128 | 0003 |
| `limits::maxResourceHeapSize` | 1,000,000 | 0029 |
| `defaults::resourceHeapSize` | 262,144 | 0029 |
| `limits::maxSamplerHeapSize` | 2048 (also the default) | 0029 |
| `limits::maxUniqueSamplers` | 4096 | 0033 |
| `limits::maxColorTargets` | 8 | 0041 |
| `limits::maxViewports` | 16 | 0037 |
| `limits::maxVertexAttributes` | 32 | 0038 |
| `limits::maxVertexBufferSlots` | 32 | 0038 |
| `limits::maxAnisotropy` | 16 | 0033 |
| `limits::maxSamplerLodBias` | 14.0f | 0033 |
| `alignment::textureCopyOffset` | 512 | 0048 |
| `alignment::textureCopyRowPitch` | 256 | 0048 |
| `limits::maxDebugNameLength` | 255 | 0017 |
| `limits::maxTypedBufferElements` | 2^27 | 0028 |
| `limits::maxStructuredBufferElements` | 2^27 | 0028 |
| `limits::maxStorageBufferViewSize` | 1 GiB | 0028 |

## Macros and build options

| Name | What it does | ADR / § |
|---|---|---|
| `SLAG_FLAGS(EnumType)` | Bitwise operators for flag enums | §10 |
| `SLAG_BACKEND_D3D12`, `SLAG_BACKEND_VULKAN` | CMake options, also defined as macros when on | §4.1 |
| `SLAG_VALIDATION` | CMake option + macro compiling validation in | §8 |
| `SLAG_DISABLE_LABELS` | Strips debug labels | 0050 |
| `SLAG_EXTERNAL_BACKEND_DIRS` | CMake variable listing private backend directories | §4.3 |
| `SLAG_EXTERNAL_BACKEND_OPTIONS` | Header adding console members to the backend-options union | 0010 |
| `SLAG_D3D12_AGILITY_SDK_EXPORTS(version, path)` | Exports `D3D12SDKVersion` / `D3D12SDKPath` from the application | 0010 |

`SLAG_EXTERNAL_SHADER_FIELDS` is not used: console shader formats add
`ShaderFormat` values instead (ADR 0036).

## Files

See CLAUDE.md §4.2 for the full layout. Public headers are flat under
`include/slag/` with `.h`, plus an umbrella `slag/Slag.h`; `native/` and
`util/` are opt-in. Small value types used by several headers (`CompareOp`,
`Viewport`, `Rect`, `ClearValue`) live in `slag/Types.h`. File names are
PascalCase, folders lowercase. Backend
sources: `src/d3d12/D3D12*.cpp`, `src/vulkan/Vk*.cpp`; shared code in
`src/common/`. CMake target `slag`,
alias `slag::slag`.

## Version macros (`slag/Version.h`, generated)

`SLAG_VERSION_MAJOR`, `SLAG_VERSION_MINOR`, `SLAG_VERSION_PATCH`,
`SLAG_VERSION` (packed `major << 22 | minor << 12 | patch`) — ADR 0070.

## CMake functions

| Name | What it does |
|---|---|
| `slag_add_shader(<target> SOURCE ENTRY STAGE)` | Compiles a Slang shader to SPIR-V / DXIL for the enabled backends (`cmake/SlagShaders.cmake`) |
| `slag_copy_agility_runtime(<target>)` | Copies the Agility SDK runtime DLLs into `D3D12\` next to an executable (`cmake/SlagDependencies.cmake`) |

## Render graph module (settled after M9)

Namespace `slag::rendergraph`; target `slag::rendergraph`; header
`slag/rendergraph/RenderGraph.h`; CMake option `SLAG_BUILD_RENDERGRAPH`;
sample `samples/render_graph` (ADR 0072).

| Name | Notes |
|---|---|
| `RenderGraph`, `createRenderGraph(RenderGraphDesc)`, `destroy(RenderGraph*)` | The graph object |
| `RenderGraphDesc` | `device`, `graphicsQueue` / `computeQueue` / `copyQueue`, `framesInFlight`, `allocatorCallbacks`, `messageCallback`, `messageUserData` |
| `beginFrame()`, `compile()`, `passCount()`, `recordPass(index)`, `execute()` | Frame steps; `execute` submits the recorded passes |
| `transientMemorySize()` | Bytes the frame's transients share |
| `importTexture(Texture*, ImportedTextureDesc)`, `importBuffer(Buffer*)` | Resources the graph doesn't own |
| `ImportedTextureDesc` | `initialLayout`, `finalLayout`, `attachmentView`, `swapchain` |
| `createTransientTexture2D(Texture2DDesc)` (and per type, below), `createTransientBuffer(BufferDesc)` | Graph-owned, one frame |
| `addPass(name, queue, setup, record)` | Setup runs now; record is stored |
| `PassBuilder`: `use(handle, Usage)`, `renderTarget(...)`, `depthStencil(...)` | Declares a pass's uses |
| `PassContext`: `texture(handle)`, `buffer(handle)`, `frameSlot()` | Given to record functions |
| `RenderGraphTexture`, `RenderGraphBuffer` | Frame-local handles (renamed in 1.1.0 from `TextureHandle`, `BufferHandle`, which stay as deprecated aliases until 2.0) |
| `Usage`: `ShaderRead`, `ShaderWrite`, `CopySource`, `CopyDest`, `VertexBuffer`, `IndexBuffer`, `ConstantBuffer`, `IndirectArgument` | How a pass uses a resource |

| `defragment(DefragmentationDesc, layoutOf)` | Requests a defragmentation pass after the frame (ADR 0072) |
| `movedResources()` | What the last `execute`'s defragmentation pass moved |

Settled after the render graph improvements (all kept in review):

| Name | Notes |
|---|---|
| `texture->format()`, `width()`, `height()`, `depth()`, `layerCount()`, `mipCount()`, `sampleCount()`, `usage()` | Texture getters (ADR 0021) |
| `createTransientTexture2D`, `createTransientTexture2DArray`, `createTransientTextureCube`, `createTransientTextureCubeArray`, `createTransientTexture3D` | Transient textures (ADR 0072); replace `createTransientTexture` |
| `use(handle, usage, SubresourceRange range = {})` | The subresources a pass uses |
| `renderTarget(..., mip, baseLayer, layerCount)`, `depthStencil(..., readOnly, mip, baseLayer, layerCount)` | Attachment subresources |

Settled before tagging v1.0 (all kept in review):

| Name | Notes |
|---|---|
| `DebugMessage` { `severity`, `source`, `text` } | What the debug callback receives (ADR 0011); `DebugMessageFn` now takes `(const DebugMessage&, void* userData)` |
| `swapchain->resize(width, height)` | Changes only the size (ADR 0058) |

Decided in the M9 review: the namespace is spelled out (not `rg`); `Usage`
rather than `Access`, so it isn't confused with `BarrierAccess`; `Handle`
rather than `Ref` (the handles were renamed `RenderGraphTexture` and
`RenderGraphBuffer` in 1.1.0, so engines can have texture handles of their
own; Crucible asked); `execute` rather than `submit`; transients say so in
their create functions. Reviewed afterwards: `defragment` kept,
`movedResources` rather than `defragmentationMoves`.

## Enum value style

PascalCase; formats spell out channel bits as in DXGI (`R8G8B8A8UnormSrgb`)
— ADR 0022.
