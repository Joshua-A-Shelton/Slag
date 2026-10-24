# 0009 — Queues

**Status:** Accepted (2026-10-22). Queue count superseded by ADR 0054 (counts per type at initialization).

## Decision

- An initialized graphics device provides **one queue of each type**:
  graphics, compute, copy.
- Slag queues may **silently share hardware**. Vulkan mapping, in order of
  preference:
  1. a dedicated queue family for the type;
  2. a second `VkQueue` from the graphics family;
  3. the same `VkQueue` as graphics (single-queue implementations, e.g.
     software rasterizers).
- When Slag queues share one `VkQueue`, submission and present take an
  internal lock (documented exception to "no hidden locks").
- No queue priorities in v1.
- All Vulkan resources use `VK_SHARING_MODE_CONCURRENT` across the queue
  families in use. No queue-family ownership transfers. (Accepted cost:
  possible loss of compression on older hardware. Confirmed in the barrier
  design, ADR 0049.)

## Known hazard

With a shared `VkQueue`, a wait on a fence value whose signal is submitted
*later* deadlocks, while it works with separate queues. Portable rule: submit
the signal before (or together with) any wait on it. Validation should
detect violations when queues are shared.
