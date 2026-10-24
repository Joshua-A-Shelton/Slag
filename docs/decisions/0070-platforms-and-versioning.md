# 0070 — Platforms, CPU architectures, versioning

**Status:** Accepted (2026-10-23). `Version.h` names are final
(docs/naming.md).

## Platforms

| | |
|---|---|
| Windows | Windows 10 and 11; D3D12 and Vulkan. Minimum Windows 10 version: whatever the pinned Agility SDK release supports. |
| Linux | Vulkan, on drivers meeting the baseline (Mesa 26.2+ RADV/ANV, NVIDIA 610+). Xlib, XCB, Wayland windows (ADR 0055). |
| Compilers | MSVC (current Visual Studio), Clang 17+, GCC 13+. |
| Out of scope | macOS, Android, iOS. Consoles are out of tree. |

## CPU architectures

- **x64:** supported and tested on both operating systems.
- **ARM64:** must compile; not supported or tested in v1. Whether Windows on
  ARM or ARM64 Linux drivers meet the baseline is an open check. No
  x86-only code without a portable fallback.
- **32-bit:** cut (64-bit pointers and GPU addresses assumed).
- **Big-endian:** cut.

## Versioning

- Semantic versioning. Before 1.0, minor releases may break the API; from
  1.0, only major releases.
- The CMake `project(VERSION ...)` is the single source; it generates
  `slag/Version.h`:

  ```cpp
  #define SLAG_VERSION_MAJOR 0
  #define SLAG_VERSION_MINOR 1
  #define SLAG_VERSION_PATCH 0
  #define SLAG_VERSION ((SLAG_VERSION_MAJOR << 22) | (SLAG_VERSION_MINOR << 12) | SLAG_VERSION_PATCH)
  ```

  Same packing as Vulkan's version encoding, so versions compare with `<`.
- No runtime version function (static library: headers and library always
  match).
- No stable ABI (ADR 0010).
- From 1.0: APIs are `[[deprecated]]` for at least one minor release before
  removal in the next major.
- `CHANGELOG.md` from the first tagged release.
