# CLAUDE.md — Slag

Slag is a thin, low-level C++ abstraction over modern explicit graphics APIs.
Primary backends: **Direct3D 12** and **Vulkan**. The architecture must leave
room for NDA-gated console backends (PlayStation, Nintendo Switch 2), which are
**not** a priority and must never be referenced by name in public code beyond
the backend enum and build options.

Licensed under zlib (`LICENSE`, ADR 0064); dependencies must stay
compatible.

The primary focus is games; tools and other applications are valid use
cases too, and design decisions shouldn't rule them out.

This file is the source of truth for project goals, constraints, and
conventions. If a request conflicts with it, point out the conflict before
writing code.

---

## 1. Goals

- **Thin.** A Slag call should map to one (or a small, predictable number of)
  native API calls. No hidden render graphs, no automatic resource tracking, no
  implicit barriers, no hidden allocations on hot paths.
- **Explicit.** The user owns synchronization, memory residency, lifetime, and
  command recording. Slag exposes concepts that exist in both D3D12 and Vulkan
  and picks the *intersection* that can be expressed without emulation.
- **Zero/low overhead.** No virtual dispatch on hot paths (command recording,
  descriptor writes). Per-call overhead should be comparable to calling the
  native API directly.
- **Portable by construction.** Anything expressible in the common interface
  must work on every shipping backend. Backend-specific features go through an
  explicit escape hatch (§6), never through silent divergence.
- **Escape hatches everywhere.** Every Slag object can hand back its native
  handle(s), and native objects can be wrapped/imported.

## 2. Non-goals

- Not a renderer, scene graph, material system, or render graph. (A render
  graph may be built *on top of* Slag in a separate module later.)
- No legacy API support: no D3D11, OpenGL, Vulkan < 1.3 (1.3 itself only
  under the fallback policy in §3), or D3D12 without Enhanced Barriers.
- No automatic state tracking or barrier inference in the core.
- No shader language of our own. No runtime shader cross-compilation in the core.
- Not thread-safe by default (see §7) — the user synchronizes access.

## 3. Platform & API baseline

| Backend | Minimum | Required features |
|---|---|---|
| D3D12 | Agility SDK 1.619.6 (retail; preview SDKs need Developer Mode, so never ship them) (app exports `D3D12SDKVersion`/`D3D12SDKPath` via a Slag macro), Shader Model 6.8 (ADR 0047) | **Enhanced Barriers** (required, no legacy resource-state fallback), `ResourceDescriptorHeap`/`SamplerDescriptorHeap` (SM 6.6 dynamic resources), DXGI 1.6 |
| Vulkan | **1.4** | **`VK_EXT_descriptor_heap`** (required), plus the 1.3/1.4 core features: dynamic rendering, synchronization2, timeline semaphores, buffer device address. Known driver support (2026-10): NVIDIA 610+; AMD Windows 25.30.17.02+ (RX 7000/9000 class only); Mesa 26.2+ RADV and ANV; lavapipe unconfirmed. Older AMD GPUs on Windows use the D3D12 backend; revisit with the 1.3 fallback evaluation |
| Vulkan fallback (maybe) | 1.3 | Only if it fits the existing API unchanged — see below |
| Console (future) | TBD | Private repos only |

**Vulkan 1.3 fallback policy.** The common interface is designed against
Vulkan 1.4 + `VK_EXT_descriptor_heap` and D3D12 only. A Vulkan 1.3 fallback
(e.g. without descriptor heaps) is evaluated **only after** that design is
settled, and is accepted only if it can implement the API **without changing
it** — no extra types, parameters, caps, or semantics leaking into the
common interface for its sake. If the fallback can't meet that bar, it
doesn't ship. Never weaken or complicate the API design to accommodate 1.3.

- Target OSes (ADR 0070): Windows 10/11 (D3D12, Vulkan; minimum Windows 10
  version set by the pinned Agility SDK), Linux (Vulkan). macOS, Android, iOS
  are out of scope (no MoltenVK target).
- Compilers: MSVC (current Visual Studio), Clang 17+, GCC 13+.
- CPU architectures: x64 supported and tested. ARM64 must compile but isn't
  supported in v1, so no x86-only code without a portable fallback. No
  32-bit, no big-endian.
- Versioning: semantic versioning (pre-1.0 minors may break the API). The
  CMake project version generates `slag/Version.h` (`SLAG_VERSION_MAJOR`,
  `_MINOR`, `_PATCH`, packed `SLAG_VERSION`). No stable ABI.
- Language: **C++20**. Avoid features with poor MSVC/console-toolchain support
  (modules, `<format>` in public headers, coroutines).

## 4. Architecture

### 4.1 Backend selection

- Backends are selected at **build time** via CMake options
  (`SLAG_BACKEND_D3D12`, `SLAG_BACKEND_VULKAN`, ...). More than one may be
  compiled in at once (e.g. Windows builds with both).
- The active backend is chosen at **instance creation time** (ADR 0008). The
  instance lists graphics devices; a listed device carries info only until
  it is initialized. Every object created from it belongs to that one
  backend; mixing is UB and asserted in debug.
- Backend-specific options are a union in the instance description, using
  plain types only (ADR 0010). In v1 the D3D12 and Vulkan members are empty.
- Dispatch (ADRs 0001, 0065): public classes have non-virtual methods and no
  visible data; backend objects derive from internal common structs. One
  X-macro list generates the per-backend function table. Every public
  function is implemented once in `src/common/` (validation, then
  `SLAG_DISPATCH`), which calls the backend directly when only one backend is
  compiled in. No vtables.
- Object references (ADR 0052, superseding 0002): every object (buffers,
  textures, attachment views, pipelines, memory blocks, query pools, fences,
  queues, command lists, swapchains, devices) is a pointer to an opaque
  object. No generational handles. Lifetime is explicit create/destroy.

### 4.2 Layout

```
CMakeLists.txt  CMakePresets.json  .clang-format  CLAUDE.md  LICENSE
cmake/                         Version.h.in, SlagDependencies.cmake, SlagShaders.cmake
include/slag/
  Slag.h                       umbrella: everything except native/ and util/
  Result.h                     Result, Status
  Flags.h                      SLAG_FLAGS
  Limits.h                     limits::, alignment::, defaults:: constants
  PixelFormat.h                PixelFormat, FormatSupport
  Instance.h                   Instance, InstanceDesc, backend-options union, debug callback
  GraphicsDevice.h             GraphicsDevice, info, capabilities, init desc, MemoryBudget
  CommandQueue.h               CommandQueue, SubmitDesc
  CommandBuffer.h              CommandBuffer, RenderingDesc, draw/copy argument structs
  Buffer.h  Texture.h  AttachmentView.h  MemoryBlock.h
  Descriptors.h                all ...Descriptor structs, samplers
  Pipeline.h                   pipeline Descs, state structs, ShaderStage, VertexInputLayout
  Barriers.h                   BarrierSync/Access, TextureLayout, barrier structs
  QueryPool.h  Semaphore.h
  Defragmentation.h            DefragmentationDesc, DefragmentationMove, DefragmentationPass
  Swapchain.h                  Swapchain, NativeWindow, PresentMode, ColorSpace, HDR metadata
  Types.h                      shared small value types: CompareOp, Viewport, Rect, ClearValue
  Version.h                    generated from the CMake project version (ADR 0070)
  native/D3D12.h               opt-in interop; includes d3d12.h
  native/Vulkan.h              opt-in interop; includes vulkan.h
  native/D3D12Agility.h        SLAG_D3D12_AGILITY_SDK_EXPORTS (no Windows headers)
  util/DescriptorAllocator.h   slot allocator utility (ADR 0032)
  util/DeferredDestroyer.h     delayed destruction utility (ADR 0069)
src/
  common/                      dispatch tables, validation, stale-pointer quarantine, format tables
  d3d12/                       D3D12*.cpp
  vulkan/                      Vk*.cpp
  util/                        the slag/util/ utilities, built on public functions
tests/
  unit/                        no GPU needed
  backend/                     runs on every compiled backend
samples/                       triangle, textured_quad, compute, bindless, multi_queue
third_party/                   pinned: Vulkan-Headers, Agility SDK (ADR 0061)
docs/                          decisions/, naming.md, open-items.md, format-support.md, milestones.md
```

External (console) backends live outside the repository, in the directories
listed in `SLAG_EXTERNAL_BACKEND_DIRS`.

- Public headers must **never** include `d3d12.h`, `vulkan.h`, `windows.h`, or
  any console SDK header. Native interop lives in opt-in headers
  (`slag/native/D3D12.h`, `slag/native/Vulkan.h`).
- Backend code may only depend on the public headers, `src/common/`, and
  its own native API.

### 4.3 Console backends

- Live out-of-tree; discovered via `SLAG_EXTERNAL_BACKEND_DIRS` in CMake.
- Registration (ADR 0065): each backend provides `slagBackend_<name>()`;
  CMake generates the registry. `Backend` values 0x80+ are reserved for
  external backends, defined in their private headers — the public repo has
  no console-specific types, values, comments, or code paths.
- Design the common interface so that console constraints (unified memory,
  fixed-function differences, different descriptor models) are *possible* to
  implement, but don't speculate beyond publicly known information.

## 5. Core design principles for the common interface

These guide the interface design phase; refine them as decisions are made.
Accepted decisions live in `docs/decisions/`.

- **Hardware, not spec.** Expose what current target hardware actually
  implements well, not everything the specs allow. Spec features that are
  rarely implemented, emulated by drivers, or irrelevant to shipping GPUs
  are cut, even if both APIs technically have them. Each feature's inclusion
  is decided with the project owner (§13).
- **Fixed pipeline layout** (ADR 0003): every pipeline implicitly uses the
  resource heap, the sampler heap, 128 bytes of push constants, and one
  constant buffer slot set by address and size (ADR 0044). No user-defined layouts, no pipeline-layout object.
- **Descriptor slots, not views** (ADR 0004): heaps have opaque,
  index-addressed slots written from a descriptor description; raw
  descriptor memory isn't exposed. There are no SRV/UAV view objects — the
  only view object is an attachment view for render/depth targets.
- **Feature staging** (ADR 0005): v1 core is graphics (VS + PS), compute,
  indirect, multi-queue. Mesh shaders then ray tracing follow as optional
  capabilities; work graphs stay native-only.
- **Placed resources and aliasing in v1** (ADR 0006), alongside committed
  creation.
- **C-shaped C++ API** (ADR 0007): no C API yet, but no templates,
  `std::function`, or other constructs that would block one.
- **Descriptor heaps, bindless-only.** Both backends use the same model: a
  resource heap and a sampler heap, with shaders indexing into them
  (D3D12 `ResourceDescriptorHeap`/`SamplerDescriptorHeap`, Vulkan
  `VK_EXT_descriptor_heap`). There are no descriptor sets, descriptor tables,
  or per-draw binding objects in the common interface. Each device owns
  exactly one heap of each kind, bound automatically on every command list
  (ADR 0029; preferred, not a hard rule — revisit if exposing heap objects
  serves another part of the API better). Slag exposes per-kind descriptor writes at an index (ADR 0031);
  slot allocation within a heap is the user's job.
- **Explicit barriers** modeled directly on D3D12 Enhanced Barriers (sync +
  access + layout, split into global, buffer, and texture barriers). Vulkan
  maps these onto synchronization2 (`vkCmdPipelineBarrier2`). The Enhanced
  Barriers model is the reference: if the two APIs disagree, follow D3D12's
  semantics and document how Vulkan is mapped.
- **Timeline fences** are the only CPU/GPU and cross-queue sync primitive
  exposed (D3D12 fences, Vulkan timeline semaphores). Binary semaphores are an
  internal detail of the Vulkan swapchain.
- **Dynamic rendering** (begin/end rendering with attachments) — no render
  pass / framebuffer objects.
- **Explicit memory.** Users can allocate memory blocks and place resources,
  or use automatic placement. Automatic placement uses Slag's own
  backend-agnostic sub-allocator (ADR 0061), designed to support
  defragmentation (ADR 0062, milestone M6).
- **Shaders are pre-compiled bytecode.** Slag accepts DXIL and SPIR-V blobs
  (plus future console formats). The supported toolchain is **Slang**,
  compiling one source to DXIL and SPIR-V with output flags that make both
  targets behave the same (ADR 0034). Vulkan SPIR-V requires
  `-capability spvDescriptorHeapEXT`,
  `-spirv-unified-descriptor-heap-stride`, and `-fvk-b-shift 0 0`. The shader compiler is a
  separate, optional tool/module.
- **Queues** (ADRs 0009, 0054): graphics, compute, and copy queues, with a
  count per type chosen at initialization (default one each); they may
  share hardware. Vulkan resources use
  `CONCURRENT` sharing, so there are no queue-family ownership transfers.
- **Descriptors of create-info structs**, not builder chains or long
  constructor argument lists. Designated initializers should read well.
- **Features as capabilities** (ADR 0015). Optional features are enabled
  automatically when supported and queried on the listed graphics device;
  using an unsupported feature is a validation error, never silent
  emulation.
- **D3D12's feature set is the reference.** An optional feature is only
  exposed if D3D12 has it and Vulkan can map it. Vulkan-only features are
  neither exposed nor enabled. Exception: out-of-bounds access and null
  descriptors follow Vulkan — both are undefined behavior, robustness stays
  off (ADR 0016). Exception: buffer GPU addresses may be used in shaders,
  gated by a capability that is false on D3D12 until it supports them
  (ADR 0025).

## 6. Escape hatches / native interop

- Every object exposes `native()` via the interop headers, returning the
  underlying handle(s) (`ID3D12Resource*`, `VkImage` + `VmaAllocation`, ...).
- Command lists expose their native command buffer so users can record native
  commands mid-stream. Document any state Slag assumes is preserved.
- Import paths exist for external native resources (e.g. interop with other
  libraries, video decode, XR runtimes).

## 7. Threading model

- Calls on different objects may run concurrently; calls on the same object
  are **externally synchronized** unless listed as thread-safe. The full
  per-call table is ADR 0068.
- Thread-safe by contract: every `create*` / `destroy()` on a device,
  descriptor writes to different slots, device queries, pipeline creation,
  recording *different* command buffers on different threads, and using
  immutable objects from many command buffers at once.
- No locks on hot paths (recording, descriptor writes, submits to separate
  hardware queues). Every internal lock is listed in ADR 0068; adding one
  means updating that list.

## 8. Error handling & validation

- **No exceptions** and **no RTTI** required. Library must build with
  `-fno-exceptions -fno-rtti` / `/EHs-c- /GR-`.
- Fallible operations return `slag::Result<T>`: a `Status` (success group
  incl. `Suboptimal`/`Timeout`, error group) plus a value on success (ADR
  0060). Defined in `slag/Result.h`; no dependency on `std::expected`.
- Programmer errors (invalid usage) are caught by Slag validation (ADR
  0067): compiled in with `SLAG_VALIDATION` and then always on, written once
  in `src/common/validation/`, reported through the debug callback, with
  `ValidationFailureAction` (`Break` by default, `Continue`, `Abort`). The
  failing call still proceeds. Release builds pay nothing.
- Slag doesn't track texture layouts or barrier correctness in v1; backend
  synchronization / GPU-based validation covers that.
- Device lost / out-of-memory are reported as results, never asserts.
- Backend validation layers (D3D12 debug layer, GPU-based validation, Vulkan
  validation layers) are a separate, runtime choice: `ValidationFlags` in
  `InstanceDesc`.

## 9. Memory & allocation

- All of Slag's CPU allocations go through `AllocatorCallbacks` in
  `InstanceDesc` (default: aligned `malloc`/`free`); driver allocations are
  not routed through it (ADR 0066). Allocation failure returns
  `OutOfHostMemory`, never aborts.
- Inside `src/`, use Slag's own containers (`Vector`, `SmallVector`,
  `HashMap`, `ObjectPool`) — never `new`/`delete` or standard containers
  with the default allocator. Only non-allocating standard headers are
  allowed in `src/` (list in ADR 0066).
- No allocations in command recording after warm-up: command buffers keep
  their internal capacity across `reset()`.
- Object lifetime is explicit: `create*` / `destroy()` (or RAII wrappers that
  call them). Slag does **not** defer destruction automatically; the optional
  `util/DeferredDestroyer.h` utility destroys objects once a semaphore value
  is reached (ADR 0069).

## 10. Coding conventions

- Namespace: `slag`; backend internals in `slag::d3d12`, `slag::vk`, etc.
- Naming: `PascalCase` types, `camelCase` functions and variables, `m_` not
  used — members are plain `camelCase`; `SLAG_UPPER_CASE` macros (macros
  only when unavoidable).
- Constants: `camelCase`, no prefix. Public ones are grouped in lowercase
  namespaces (`limits::maxTexture2DSize`, `alignment::constantBuffer`,
  `defaults::resourceHeapSize`) or are `static constexpr` members of the
  type they belong to (`SubresourceRange::remainingMips`).
- Enums: `enum class`; bitflags via a `SLAG_FLAGS(EnumType)` helper providing
  operators.
- Enum values are PascalCase. Format values spell out channel bits as in
  DXGI with a PascalCase type suffix (`R8G8B8A8UnormSrgb`, `D32FloatS8Uint`,
  `BC7UnormSrgb`) — ADR 0022.
- Files: `PascalCase.h` / `PascalCase.cpp`, named after the type or topic
  they hold (`CommandBuffer.h`). Folders stay lowercase (`include/slag/`,
  `native/`, `util/`, `src/common/`). Public headers are flat under
  `include/slag/`, one per object or topic. Backend sources carry a `D3D12`
  or `Vk` prefix (`D3D12CommandBuffer.cpp`, `VkCommandBuffer.cpp`).
  Includes are always `#include <slag/...>`, inside Slag too.
- Formatting: `.clang-format` in repo root is authoritative. Run it before
  committing.
- Header hygiene: public headers are self-contained, minimal includes, forward
  declare where possible. Keep compile times low.
- Prefer `std::span`, `std::string_view`, and plain structs in the public API.
  Public headers may include only `<cstdint>`, `<cstddef>`, `<span>`, and
  `<string_view>` from the standard library (ADR 0066).
- Comments explain *why* and any D3D12/Vulkan mapping subtleties, not *what*.

## 11. Build & tooling

- **CMake** (3.25+) with presets (`CMakePresets.json`). Library target
  `slag`, alias `slag::slag`. Slag builds as a static library by default;
  shared library support is optional.
- Keep dependencies low (ADR 0061). The library depends only on
  Vulkan-Headers (v1.4.366) and the DirectX 12 Agility SDK (1.619.6),
  fetched at configure time by `cmake/SlagDependencies.cmake`, pinned by
  version and SHA-256. Nothing else in the library without discussion.
  Offline builds: set `FETCHCONTENT_SOURCE_DIR_<NAME>`.
- Tests use doctest (v2.5.3), registered with CTest via
  `doctest_discover_tests`.
- Shaders for tests and samples are built with
  `slag_add_shader(<target> SOURCE ... ENTRY ... STAGE compute|vertex|pixel)`
  (`cmake/SlagShaders.cmake`): SPIR-V when the Vulkan backend is on (checked
  with `spirv-val` if found), DXIL when the D3D12 backend is on (needs DXC,
  so Windows only). Tested with Slang 2026.18.2.
- Slag implements its own Vulkan loader (per-device function tables, no
  link-time dependency on `vulkan-1`), its own memory sub-allocator for both
  backends, and its own SPIR-V input reflection.
- Tests, samples, and tools may use a unit test framework, a windowing
  library, and an image loader; these are never linked into the library.

```bash
cmake --preset debug && cmake --build --preset debug
```

```bash
ctest --preset debug
```

Presets: `debug` (validation on) and `release`, Ninja, output in
`build/<preset>/`. CMake options: `SLAG_BACKEND_D3D12` (default on for
Windows), `SLAG_BACKEND_VULKAN`, `SLAG_VALIDATION`, `SLAG_DISABLE_LABELS`,
`SLAG_BUILD_TESTS`, `SLAG_BUILD_SAMPLES`, `SLAG_EXTERNAL_BACKEND_DIRS`.
The `header_check` test compiles every public header on its own, except
`native/D3D12.h` and `native/Vulkan.h`.

## 12. Testing

- Unit tests for backend-agnostic code (formats, flag helpers, validation
  logic) run without a GPU.
- **CI is split** (GitHub Actions, `.github/workflows/ci.yml`):
  - Hosted CI, on every change: Linux (GCC, Clang) and Windows (MSVC)
    builds in both presets, unit tests, a clang-format check (pinned
    clang-format), and later D3D12 backend tests on WARP. Slang, and DXC on
    Windows, are fetched pinned by SHA-256. Linux CI turns
    `SLAG_VALIDATE_SPIRV` off until runners have SPIRV-Tools 2026.3+.
  - A self-hosted GPU runner (Linux, AMD, RADV / Mesa 26.2+), on every merge
    or nightly: Vulkan backend tests.
  - No software Vulkan driver supports `VK_EXT_descriptor_heap` yet; re-check
    lavapipe periodically and move Vulkan tests to hosted CI once it does.
- Every common-interface feature needs a test that runs on **all** compiled
  backends and ideally compares output images across them.
- Samples (`samples/`) double as smoke tests: triangle, textured quad,
  compute, bindless, multi-queue.

## 13. Working with Claude on this repo

- **Ask before naming.** Propose names for public types, functions, enums,
  enum values, and files, but don't settle them — the project owner decides.
  Until then, mark names as placeholders. Final names are recorded in
  `docs/naming.md`.
- **Ask before choosing features.** For any API surface (formats, usage
  flags, pipeline state, sampler options, barrier sync/access values, query
  types, etc.), present what both specs offer, recommend a hardware-relevant
  subset, and wait for a decision. Default to cutting, not including.
- When adding a common-interface feature, state how it maps to **both** D3D12
  and Vulkan before implementing. If one side needs emulation, flag it — it
  may not belong in the common interface.
- Never add a public-header dependency on a native API or console SDK.
- Keep changes minimal and consistent with existing code; don't introduce new
  dependencies or abstractions without asking.
- Record design decisions in `docs/decisions/` (short ADR-style notes) and
  update this file when a decision changes a rule here.

## 14. Open questions

Questions 1–7 from the first design pass are resolved; see
`docs/decisions/0001`–`0007`.

The per-draw root constant buffer question was resolved by ADR 0044, and
static samplers were cut in ADR 0033. Everything else still open (owner
decisions, spec/driver checks, deliverables) is tracked in
`docs/open-items.md`.

1. *(After the interface is settled.)* Can a Vulkan 1.3 backend implement the
   API unchanged (§3 fallback policy)? If not, drop it. A fallback would also
   cover older AMD GPUs on Windows Vulkan, which lack `VK_EXT_descriptor_heap`.
