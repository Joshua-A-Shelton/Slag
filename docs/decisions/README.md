# Design decisions

Short ADR-style notes. One decision per file, numbered in order. Each note has
a status (`Accepted`, `Superseded by NNNN`, `Proposed`), the decision, how it
maps to D3D12 and Vulkan, and why.

Type and function names in these notes are **placeholders** unless a note
says a name is final. Names are chosen in a separate, explicit step.

| # | Decision | Status |
|---|---|---|
| [0001](0001-backend-dispatch.md) | Backend dispatch | Accepted |
| [0002](0002-handle-style.md) | Handle style | Superseded by 0052 |
| [0003](0003-fixed-pipeline-layout.md) | Fixed pipeline layout | Accepted |
| [0004](0004-descriptor-writes.md) | Descriptor writes and views | Accepted |
| [0005](0005-advanced-feature-staging.md) | Mesh / RT / work graph staging | Accepted |
| [0006](0006-placed-resources.md) | Placed resources and aliasing | Accepted |
| [0007](0007-no-c-api.md) | No C API (for now) | Accepted |
| [0008](0008-instance-and-graphics-devices.md) | Instance and graphics devices | Accepted |
| [0009](0009-queues.md) | Queues | Accepted |
| [0010](0010-backend-options.md) | Backend-specific options | Accepted |
| [0011](0011-debug-callback.md) | Debug message callback | Accepted |
| [0012](0012-device-filtering.md) | Filtering unsupported devices | Accepted |
| [0013](0013-presentation-support.md) | Presentation support | Accepted |
| [0014](0014-crash-diagnostics.md) | Crash diagnostics | Accepted |
| [0015](0015-optional-features.md) | Optional features | Accepted |
| [0016](0016-robustness.md) | Robustness and null descriptors | Accepted |
| [0017](0017-debug-names.md) | Debug names | Accepted |
| [0018](0018-native-device-import.md) | Native device import (deferred) | Accepted |
| [0019](0019-memory-kinds.md) | Memory kinds | Accepted |
| [0020](0020-buffers.md) | Buffers | Accepted |
| [0021](0021-textures.md) | Textures | Accepted |
| [0022](0022-formats.md) | Formats | Accepted |
| [0023](0023-resource-creation.md) | Resource creation and placement | Accepted |
| [0024](0024-cpu-mapping.md) | CPU mapping | Accepted |
| [0025](0025-gpu-addresses.md) | GPU addresses | Accepted |
| [0026](0026-sparse-resources.md) | Sparse resources (cut from v1) | Accepted |
| [0027](0027-external-memory.md) | External memory (cut from v1) | Accepted |
| [0028](0028-resource-limits.md) | Resource limits | Accepted |
| [0029](0029-descriptor-heap-ownership.md) | Descriptor heap ownership and sizes | Accepted |
| [0030](0030-descriptor-types.md) | Resource descriptor types | Accepted |
| [0031](0031-descriptor-writes.md) | Writing descriptors | Accepted |
| [0032](0032-slot-allocator.md) | Descriptor slot allocator utility | Accepted |
| [0033](0033-samplers.md) | Samplers | Accepted |
| [0034](0034-shader-access.md) | Shader language and heap access | Accepted |
| [0035](0035-attachment-views.md) | Attachment views | Accepted |
| [0036](0036-shader-input.md) | Shader input to pipelines | Accepted |
| [0037](0037-pipeline-kinds-and-dynamic-state.md) | Pipeline kinds and dynamic state | Accepted |
| [0038](0038-vertex-input.md) | Vertex input | Accepted |
| [0039](0039-rasterizer-state.md) | Rasterizer state | Accepted |
| [0040](0040-depth-stencil-state.md) | Depth and stencil state | Accepted |
| [0041](0041-blend-state.md) | Blend state | Accepted |
| [0042](0042-target-formats-and-topology.md) | Render target formats and topology | Accepted |
| [0043](0043-pipeline-caching.md) | Pipeline caching | Accepted |
| [0044](0044-root-constant-buffer.md) | Root constant buffer slot | Accepted |
| [0045](0045-command-lists.md) | Command list allocation and reuse | Accepted |
| [0046](0046-render-passes.md) | Render passes | Accepted |
| [0047](0047-draws-and-dispatches.md) | Draws and dispatches | Accepted |
| [0048](0048-copies-and-clears.md) | Copies and clears | Accepted |
| [0049](0049-barriers.md) | Barriers | Accepted |
| [0050](0050-debug-labels.md) | Debug labels | Accepted |
| [0051](0051-queries.md) | Queries | Accepted |
| [0052](0052-opaque-pointers.md) | Opaque pointers for all objects | Accepted |
| [0053](0053-fences-and-submission.md) | Fences and submission | Accepted |
| [0054](0054-queue-counts-and-device-loss.md) | Queue counts and device loss | Accepted |
| [0055](0055-window-input.md) | Swapchain window input | Accepted |
| [0056](0056-swapchain-creation.md) | Swapchain creation options | Accepted |
| [0057](0057-acquire-and-present.md) | Acquire and present | Accepted |
| [0058](0058-resizing.md) | Resizing and reconfiguring swapchains | Accepted |
| [0059](0059-fullscreen-tearing-hdr.md) | Fullscreen, tearing, HDR | Accepted |
| [0060](0060-result-and-status.md) | Result and Status | Accepted |
| [0061](0061-dependencies.md) | Third-party dependencies | Accepted |
| [0062](0062-defragmentation.md) | Defragmentation | Accepted |
| [0063](0063-user-data.md) | User data on objects | Accepted |
| [0064](0064-license.md) | License (zlib) | Accepted |
| [0065](0065-dispatch-and-registration.md) | Dispatch and backend registration | Accepted |
| [0066](0066-cpu-allocator-and-containers.md) | CPU allocator and containers | Accepted |
| [0067](0067-validation.md) | How validation is built | Accepted |
| [0068](0068-thread-safety.md) | Per-call thread safety | Accepted |
| [0069](0069-deferred-destroyer.md) | Delayed destruction utility | Accepted |
| [0070](0070-platforms-and-versioning.md) | Platforms, CPU architectures, versioning | Accepted |
