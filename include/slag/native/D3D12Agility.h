#pragma once

// SLAG_D3D12_AGILITY_SDK_EXPORTS(version, path): exports D3D12SDKVersion and
// D3D12SDKPath from the application (ADR 0010). Needs no Windows headers.
//
// Use once, at namespace scope in one source file of the executable; the
// path is relative to it and holds D3D12Core.dll:
//
//     SLAG_D3D12_AGILITY_SDK_EXPORTS(619, ".\\D3D12\\")
//
// The version must match the Agility SDK Slag was built with.

#ifdef _WIN32
#define SLAG_D3D12_AGILITY_SDK_EXPORTS(version, path)                                                                  \
    extern "C" {                                                                                                       \
    __declspec(dllexport) extern const unsigned int D3D12SDKVersion = version;                                         \
    __declspec(dllexport) extern const char* D3D12SDKPath = path;                                                      \
    }
#else
#define SLAG_D3D12_AGILITY_SDK_EXPORTS(version, path)
#endif
