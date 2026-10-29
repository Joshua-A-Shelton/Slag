#Slag

A thin,
    low - level C++ 20 abstraction over Direct3D 12 and Vulkan.

                                                        Slag keeps the explicit model of modern graphics APIs
    : you own synchronization,
    memory, lifetime, and command recording.It exposes what both APIs share and can express without emulation,
    maps each call to one or a few native calls,
    and has no virtual dispatch on hot paths.It is not a renderer or a render graph;
those can be built on top.

    - **Bindless only.**One resource heap and one sampler heap per device; shaders
  index them (`ResourceDescriptorHeap[i]`). No descriptor sets or layouts.
- **Fixed pipeline layout.** 128 bytes of push constants and one constant
  buffer by address, for every pipeline.
- **Enhanced Barriers.** Sync, access, and layout barriers modeled on D3D12;
  Slag inserts none of its own.
- **Timeline semaphores** for every CPU/GPU and queue-to-queue wait.
- **Explicit memory**: automatic placement in Slag's sub-allocator, your own
  memory blocks, aliasing, and defragmentation passes.
- **Validation** of every call in validation builds, with a stale-pointer
  quarantine;
  release builds pay nothing.-
          **Escape hatches** : every object hands back its native handles(`slag / native / D3D12.h`, `slag / native /
                                                                                                         Vulkan.h`)
                                   .

                               Licensed under zlib(`LICENSE`)
                                   .

                               ##Requirements

      | | Minimum | | -- -| -- -| | D3D12 | Windows 10 / 11,
      Agility SDK 1.619.6(fetched at configure time), Shader Model 6.8,
      Enhanced Barriers | | Vulkan
          | Vulkan 1.4 with `VK_EXT_descriptor_heap` and `VK_KHR_shader_untyped_pointers`: NVIDIA 610 +
      , AMD Windows 25.30.17.02 + (RX 7000 / 9000),
      Mesa 26.2 + RADV and ANV.Older AMD GPUs on Windows use D3D12 | | Platforms | Windows(D3D12, Vulkan),
      Linux(Vulkan).x64;
  ARM64 compiles but isn't supported | | Compilers | MSVC(current Visual Studio), Clang 17 +
      , GCC 13 + | | Build | CMake 3.25 +, Ninja; Slang 2026.18.2 for the tests' and samples' shaders (DXC too on Windows) |

Devices that miss the baseline aren't listed; the debug callback says why.

## Building

```bash
cmake --preset debug && cmake --build --preset debug
```

```bash
ctest --preset debug
```

Presets: `debug` (Slag validation on) and `release`, output in
`build/<preset>/`. Options: `SLAG_BACKEND_D3D12` (on by default on Windows),
`SLAG_BACKEND_VULKAN`, `SLAG_VALIDATION`, `SLAG_DISABLE_LABELS`,
`SLAG_BUILD_TESTS`, `SLAG_BUILD_SAMPLES`. Link `slag::slag`.

Executables on D3D12 export the Agility SDK version from one source file and
ship `D3D12Core.dll` next to them:

```cpp
#include <slag/native/D3D12Agility.h>
SLAG_D3D12_AGILITY_SDK_EXPORTS(619, ".\\D3D12\\")
```

## Shaders

Slag takes precompiled DXIL and SPIR-V. Compile one Slang source to both; for
      SPIR - V, pass

``` - capability spvDescriptorHeapEXT - spirv - unified - descriptor - heap - stride - fvk - b - shift 0 0 - fvk -
                    use - entrypoint -
                    name
```

                        so both backends see the same heaps and the root constant buffer at
`register(b1)`.CMake projects can use `slag_add_shader` from
`cmake / SlagShaders.cmake`.

                        ##A first dispatch

                        A compute shader writes through the resource heap;
  the CPU reads the result back.

```slang struct Push
  {
      uint buffer;
      uint count;
  };
  [[vk::push_constant]] ConstantBuffer<Push> g_push;

  [shader("compute")][numthreads(64, 1, 1)] void main(uint3 id : SV_DispatchThreadID) {
      RWStructuredBuffer<uint> output = ResourceDescriptorHeap[g_push.buffer];
      if (id.x < g_push.count)
          output[id.x] = id.x * 2 + 1;
  }
```

```cpp
#include <slag/Slag.h>

      slag::Instance* instance = slag::createInstance({.backend = slag::Backend::Vulkan}).value();
  slag::GraphicsDevice* device = instance->graphicsDevices()[0];
  device->initialize({});

  slag::Pipeline* pipeline = device->createComputePipeline({.compute = {.binaries = binaries, .entryPoint = "main"}})
                                 .value(); // DXIL and/or SPIR-V

  constexpr uint32_t count = 1024;
  slag::Buffer* output =
      device->createBuffer({.size = count * 4, .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly)})
          .value();
  slag::Buffer* readback =
      device->createBuffer({.size = count * 4, .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
          .value();
  device->writeRWStructuredBuffer(5, {.buffer = output, .count = count, .stride = 4}); // slot 5

  slag::CommandBuffer* commands = device->createCommandBuffer({.queueType = slag::CommandQueueType::Compute}).value();
  commands->begin();
  commands->bindPipeline(pipeline);
  const uint32_t push[] = {5, count};
  commands->pushConstants(push, sizeof(push));
  commands->dispatch(count / 64, 1, 1);
  slag::BufferBarrier toCopy = {.syncBefore = slag::BarrierSync::ComputeShading,
                                .syncAfter = slag::BarrierSync::Copy,
                                .accessBefore = slag::BarrierAccess::ShaderWrite,
                                .accessAfter = slag::BarrierAccess::CopySource,
                                .buffer = output};
  commands->barrier({}, {&toCopy, 1});
  commands->copyBuffer(readback, 0, output, 0, count * 4);
  commands->end();

  slag::Semaphore* done = device->createSemaphore({}).value();
  slag::SemaphoreValue signal = {done, 1};
  slag::CommandBuffer* list[] = {commands};
  device->commandQueue(slag::CommandQueueType::Compute)->submit({.commandBuffers = list, .signals = {&signal, 1}});
  done->wait(1, UINT64_MAX);
  const auto* values = static_cast<const uint32_t*>(readback->mappedData()); // 1, 3, 5, ...
  ```

## Render graph (optional)

`slag::rendergraph` (`slag/rendergraph/RenderGraph.h`, CMake option
`SLAG_BUILD_RENDERGRAPH`) is built on the public API. Passes declare how they
use textures and buffers; the graph records every barrier, splits passes into
submits with semaphores between queues, begins and ends rendering for passes
with attachments, and places transient resources in shared memory. It never
reorders passes, and passes may be recorded on several threads. It can also
run a defragmentation pass after a frame (`graph->defragment(...)`), supplying
the layouts and submit boundary the pass needs.

```cpp
graph->beginFrame();
slag::rendergraph::RenderGraphTexture target = graph->createTransientTexture2D({.format = ..., .width = w, .height = h,
                                                    .usage = slag::TextureUsage::RenderTarget});
graph->addPass("scene", slag::CommandQueueType::Graphics,
    [&](slag::rendergraph::PassBuilder& pass) { pass.renderTarget(target, slag::LoadOp::Clear); },
    [=](slag::rendergraph::PassContext& context, slag::CommandBuffer* commandBuffer) { /* draws */ });
graph->compile();
for (uint32_t i = 0; i < graph->passCount(); ++i) graph->recordPass(i); // or on worker threads
graph->execute();
```

Everything is destroyed explicitly with `device->destroy(object)`, then
`device->shutdown()` and `slag::destroy(instance)`. `util/DeferredDestroyer.h`
destroys objects once a semaphore value is reached, and
`util/DescriptorAllocator.h` hands out heap slots.

## Samples

`samples/`: `triangle`, `textured_quad`, `compute` (a compute shader animates
a texture), `bindless` (1024 quads picking textures by index while textures
stream in and out), `multi_queue` (compute, copy, and graphics queues
overlapping, measured with timestamps), `render_graph` (the optional render
graph module driving async compute and the swapchain), and `crash` (loses the device on
purpose to show crash reports; run by hand). Each takes `--backend d3d12|vulkan`
and `--frames N`.

## Documentation

- The public headers in `include/slag/` document each call, its rules, and
  how it maps to D3D12 and Vulkan where that matters.
- `docs/decisions/`: the design decisions (ADRs), with the reasons.
- `docs/format-support.md`: guaranteed format support.
- `docs/naming.md`: every public name.
- `docs/milestones.md` and `docs/open-items.md`: what was built and tested,
  and what is left.
- `CHANGELOG.md`: release notes.
- `CLAUDE.md`: the project's goals, constraints, and conventions.
