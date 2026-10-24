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
| Waiting | `waitSemaphores(span<SemaphoreValue>, WaitMode, timeoutNs)`, `waitIdle()` |
| State and info | `status()`, `memoryBudget()` → `MemoryBudget`, `pipelineCacheData()`, `formatSupport(PixelFormat)` → `FormatSupport` |

- Every `write*` takes `(index, …Descriptor)` and has a batch overload taking
  a span of `{index, …Descriptor}`.
- Write structs end in `Descriptor` (`StructuredBufferDescriptor`,
  `TextureDescriptor`, `SamplerDescriptor`): they describe GPU descriptors,
  the one thing the word is reserved for.

## Other object methods

| Object | Methods |
|---|---|
| `Swapchain` | `acquire()` → `Result<uint32_t>` (ADR 0060), `texture(index)`, `attachmentView(index)`, `imageCount()`, `switchablePresentModes()`, `reconfigure(desc)`, `setHdrMetadata(HdrMetadata)`, `setMaxFrameLatency(n)`, `waitForNextFrame(timeoutNs)` |
| `Semaphore` | `value()`, `signal(value)` |
| `Buffer` | `mappedData()`, `gpuAddress()` |
| `SemaphoreValue` | `{ Semaphore* semaphore; uint64_t value; }` |

## Nested structs

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

## Constants (`namespace slag`)

| Name | Value | ADR |
|---|---|---|
| `kMaxTexture1DSize` | 16384 | 0028 |
| `kMaxTexture2DSize` | 16384 | 0028 |
| `kMaxTexture3DSize` | 2048 | 0028 |
| `kMaxTextureCubeSize` | 16384 | 0028 |
| `kMaxTextureArrayLayers` | 2048 | 0028 |
| `kMaxMipLevels` | 15 | 0028 |
| `kMaxSampleCount` | 8 | 0021 |
| `kMaxConstantBufferSize` | 65536 | 0028 |
| `kConstantBufferAlignment` | 256 | 0020 |
| `kRawBufferAlignment` | 256 | 0030 |
| `kMaxPushConstantsSize` | 128 | 0003 |
| `kMaxResourceHeapSize` | 1,000,000 | 0029 |
| `kDefaultResourceHeapSize` | 262,144 | 0029 |
| `kMaxSamplerHeapSize` | 2048 (also the default) | 0029 |
| `kMaxUniqueSamplers` | 4096 | 0033 |
| `kMaxColorTargets` | 8 | 0041 |
| `kMaxViewports` | 16 | 0037 |
| `kMaxVertexAttributes` | 32 | 0038 |
| `kMaxVertexBufferSlots` | 32 | 0038 |
| `kMaxAnisotropy` | 16 | 0033 |
| `kMaxSamplerLodBias` | 14.0f | 0033 |
| `kTextureCopyOffsetAlignment` | 512 | 0048 |
| `kTextureCopyRowPitchAlignment` | 256 | 0048 |
| `kMaxDebugNameLength` | 255 | 0017 |
| `kMaxTypedBufferElements` | 2^27 | 0028 |
| `kMaxStructuredBufferElements` | 2^27 | 0028 |
| `kMaxStorageBufferViewSize` | 1 GiB | 0028 |

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

## Enum value style

PascalCase; formats spell out channel bits as in DXGI (`R8G8B8A8UnormSrgb`)
— ADR 0022.
