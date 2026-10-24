# 0011 — Debug message callback

**Status:** Accepted (2026-10-22). Names are placeholders.

## Decision

One callback in the instance description: function pointer plus `void*`
user data. It carries both backend messages and Slag's own messages.

```cpp
enum class MessageSeverity : uint8_t { Error, Warning, Info };
enum class MessageSource   : uint8_t { Slag, Backend };
using DebugMessageFn = void (*)(MessageSeverity, MessageSource, const char* message, void* userData);
```

| Part | D3D12 (`ID3D12InfoQueue1`) | Vulkan (`VK_EXT_debug_utils`) |
|---|---|---|
| Error | Corruption, Error | Error |
| Warning | Warning | Warning |
| Info | Info | Info |
| Not delivered | Message | Verbose |

- `message` is valid only for the duration of the call.
- Not exposed: message IDs, categories/types, object lists, abort return
  (Slag always returns false), native filtering. Users filter in the callback.

## Behavior

- May be called from any thread, including driver threads.
- Attached during instance creation, so instance-creation errors are reported.
- With no callback set and validation on, errors and warnings go to
  `stderr` / `OutputDebugString`. With validation off, nothing is printed.
