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

## Not in v1

- Slag-written portable breadcrumbs and a structured report object (could
  be added later without changing this design).
- Vendor tools (Aftermath, Radeon GPU Detective) and vendor marker
  extensions; usable through native interop.
- Command-list debug labels: decided in the command-list design. They would
  improve these reports.
