# 0060 — Result and Status

**Status:** Accepted (2026-10-23). Names final (docs/naming.md).

## Decision

One `Status` enum with a success group and an error group (like `VkResult`).
`Result<T>` holds a `Status` and, on success, a `T`. `Result<void>` for
calls with no value.

```cpp
enum class Status : int32_t {
    // success: the value is valid
    Ok = 0,
    Suboptimal,         // acquire/present worked; reconfigure soon
    Occluded,           // present skipped: window not visible
    Timeout,            // waitSemaphores ran out of time
    // errors: no value
    OutOfDate = -1,     // swapchain must be reconfigured
    DeviceLost = -2,
    OutOfDeviceMemory = -3,
    OutOfHostMemory = -4,
    Unsupported = -5,   // format, present mode, window, ... not supported
};

template <class T> class Result {
public:
    explicit operator bool() const;   // true for every success status
    Status status() const;
    T& value();                       // only valid when true
};
```

- `if (!r)` covers every error; non-error statuses are read with
  `r.status()`, so `Swapchain::acquire()` returns `Result<uint32_t>`.
- Programmer errors are not statuses: they stay validation asserts (§8).
- Defined in `slag/core`; no dependency on `std::expected`.
