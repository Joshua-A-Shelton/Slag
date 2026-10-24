# 0065 — Dispatch and backend registration

**Status:** Accepted (2026-10-23). Refines ADR 0001. Internal names
(`BufferCommon`, `SLAG_DISPATCH`, …) are implementation details, not public
API.

## Object layout

Public headers declare classes with non-virtual member functions and no
visible data. Slag allocates backend types derived from them; common data
lives in an internal struct in between.

```cpp
// include/slag/Buffer.h (public)
class Buffer
{
public:
    uint64_t gpuAddress() const;
    // ...
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

protected:
    Buffer() = default;   // only Slag creates these
    ~Buffer() = default;
};

// src/common/Objects.h (internal)
struct BufferCommon : Buffer
{
    GraphicsDeviceCommon* device;
    uint64_t userData;
    // validation-only state
};

// src/vulkan/VkBuffer.h (backend)
struct VkBufferObject : BufferCommon
{
    VkBuffer buffer;
    // ...
};
```

No vtables. Pointers stay stable, so defragmentation can swap native data in
place (ADR 0062).

## Function table

- One list of every backend entry point (an X-macro,
  `src/common/BackendFunctions.inl`) generates the `BackendFunctions` struct
  of function pointers. Each backend fills one static table; a missing entry
  fails to compile.
- `InstanceCommon` and `GraphicsDeviceCommon` point at their backend's table.
  `CommandBufferCommon` keeps its own copy so recording calls dispatch in
  one load.

## Call path

Every public function is implemented once in `src/common/`: validation
(validation builds only), then dispatch.

```cpp
void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    auto* self = static_cast<CommandBufferCommon*>(this);
    SLAG_VALIDATE(validateDraw(self, vertexCount, instanceCount));
    SLAG_DISPATCH(self, cmdDraw)(self, vertexCount, instanceCount, firstVertex, firstInstance);
}
```

- `SLAG_DISPATCH` is a direct call into the backend when exactly one backend
  is compiled in (inlinable with LTO), and an indirect call through the table
  otherwise.
- Validation lives in this common layer, written once for all backends.

## Backend registration

- Each backend (built-in or external) provides
  `const BackendInfo* slagBackend_<name>()`. `BackendInfo` holds the
  `Backend` value, a display name, the function table, and `createInstance`.
- CMake generates `BackendRegistry.cpp`, calling every enabled backend's
  function. External backends from `SLAG_EXTERNAL_BACKEND_DIRS` declare
  their name in their own CMake file. No self-registering globals (static
  libraries drop them).
- `slag::createInstance(desc)` looks up `desc.backend`; `Unsupported` if not
  compiled in.
- `slag::isBackendAvailable(Backend)` lets applications choose at runtime.
- `Backend` values from 0x80 up are reserved for external backends, defined
  in their private headers. The public enum never names a console.
