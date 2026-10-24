# 0032 — Descriptor slot allocator utility

**Status:** Accepted (2026-10-23). Names are placeholders.

## Decision

An optional utility outside the core API (e.g.
`slag/util/DescriptorAllocator.h`), built only on public Slag functions.

- Allocates **single slots** within a user-chosen range of a heap; the rest
  of the heap stays user-managed.
- **Deferred reuse:** a slot is freed with a fence value and only handed out
  again after that fence completes (exact form follows the fence design).
- **Never hands out slot 0**, so user data can use 0 for "no descriptor".
  This is a convention of the utility only.
- **No internal locking** (§7 threading model).

## Driver-reserved heap range (verified 2026-10-23)

`VK_EXT_descriptor_heap` requires a reserved range in each bound heap
(`VkBindHeapInfoEXT::reservedRangeOffset/Size`): up to 2^15 descriptors for
the resource heap, and 96 samplers for the sampler heap without embedded
samplers. Slag allocates it beyond the user's requested size and hides it;
user slot indices are unaffected.
