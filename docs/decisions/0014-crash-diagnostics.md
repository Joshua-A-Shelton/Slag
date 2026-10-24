# 0014 — Crash diagnostics

**Status:** Accepted (2026-10-22)

## Decision

- One common instance option turns crash diagnostics on. Off by default.
- It enables the native mechanism:
  - **D3D12:** DRED auto-breadcrumbs and page-fault reporting (process-wide
    settings, applied before device creation).
  - **Vulkan:** `VK_EXT_device_fault`, when the device supports it.
    Otherwise a Warning is logged at initialization.
- The first time Slag observes device loss (fence wait, submit, or present),
  it collects what the backend provides and sends it through the debug
  callback as a single Error message (source Backend). No new API types.

## Expected content

| | D3D12 | Vulkan |
|---|---|---|
| Breadcrumbs | per command list | none |
| Page fault | address + nearby live/freed resources | address + access type + description |

## Implementation (M7, 2026-10-24)

- `describeDeviceLoss` per backend; the common device-loss path calls it the
  first time a device is lost and sends the text unformatted (no length
  limit). D3D12 queues are named "slag graphics queue 0" etc. for the
  reports.
- `samples/crash` loses the device on purpose (a shader writes through the
  descriptor of a buffer whose 1 GB memory block was destroyed) and is never
  run by CTest. On the RTX 2070 SUPER (driver 621.01):
  - **Vulkan:** an invalid write at the freed address (4 KB precision) and
    the faulting instruction address.
  - **D3D12:** NVIDIA reports the fault as a hang (`DXGI_ERROR_DEVICE_HUNG`)
    with no DRED page-fault data, and breadcrumbs mark the list finished
    (the fault surfaces after the GPU logs the dispatch as done). The report
    shows the command list, its queue, and its last operations. Breadcrumb
    contexts (label text) came back empty.
  - A small freed block's address range was reused for DRED's own
    breadcrumb memory, which the writes then corrupted; the sample writes
    only into the top quarter of a large block to avoid that.

## Not in v1

- Slag-written portable breadcrumbs and a structured report object (could
  be added later without changing this design).
- Vendor tools (Aftermath, Radeon GPU Detective) and vendor marker
  extensions; usable through native interop.
- Command-list debug labels: decided in the command-list design. They would
  improve these reports.
