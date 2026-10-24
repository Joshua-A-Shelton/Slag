# Pinned third-party dependencies (ADR 0061). Each is fetched at configure
# time and verified by hash. For offline builds, point
# FETCHCONTENT_SOURCE_DIR_<NAME> at a local copy.

include(FetchContent)

# ---------------------------------------------------------------------------
# Vulkan-Headers (Apache-2.0 OR MIT): Vulkan backend
# ---------------------------------------------------------------------------

if(SLAG_BACKEND_VULKAN)
    FetchContent_Declare(VulkanHeaders
        URL https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.366.tar.gz
        URL_HASH SHA256=577b90c208e6fd39ec9a1460e48772f63c76f6f5afb441df2a977a0fdb3f618c)
    set(VULKAN_HEADERS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(VULKAN_HEADERS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(VulkanHeaders)
endif()

# ---------------------------------------------------------------------------
# DirectX 12 Agility SDK (Microsoft license): D3D12 backend
# Retail release only; preview SDKs need Windows Developer Mode.
# ---------------------------------------------------------------------------

if(SLAG_BACKEND_D3D12)
    set(SLAG_AGILITY_SDK_VERSION 619)   # D3D12SDKVersion exported by applications
    FetchContent_Declare(AgilitySDK
        URL https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.619.6
        DOWNLOAD_NAME Microsoft.Direct3D.D3D12.1.619.6.zip
        URL_HASH SHA256=08f0489281401aa430fc37322d6c3fc98a8025175aacd714c10d562f4963f1e9)
    FetchContent_MakeAvailable(AgilitySDK)

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
        set(_slag_agility_arch arm64)
    else()
        set(_slag_agility_arch x64)
    endif()

    # The Agility headers must come before the Windows SDK's d3d12.h.
    add_library(slag_agility_sdk INTERFACE)
    target_include_directories(slag_agility_sdk BEFORE INTERFACE "${agilitysdk_SOURCE_DIR}/build/native/include")
    set(SLAG_AGILITY_SDK_BIN_DIR "${agilitysdk_SOURCE_DIR}/build/native/bin/${_slag_agility_arch}"
        CACHE INTERNAL "Agility SDK runtime DLLs")
endif()

# Copies the Agility SDK runtime next to an executable, in the D3D12\ folder
# that applications name with D3D12SDKPath (SLAG_D3D12_AGILITY_SDK_EXPORTS).
function(slag_copy_agility_runtime target)
    if(NOT SLAG_BACKEND_D3D12)
        return()
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/D3D12"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SLAG_AGILITY_SDK_BIN_DIR}/D3D12Core.dll"
            "${SLAG_AGILITY_SDK_BIN_DIR}/d3d12SDKLayers.dll"
            "$<TARGET_FILE_DIR:${target}>/D3D12/"
        VERBATIM)
endfunction()

# ---------------------------------------------------------------------------
# doctest (MIT): tests only, never linked into the library
# ---------------------------------------------------------------------------

if(SLAG_BUILD_TESTS)
    FetchContent_Declare(doctest
        URL https://github.com/doctest/doctest/archive/refs/tags/v2.5.3.tar.gz
        URL_HASH SHA256=174ebc4e769928959614789c5b4e9c3d0a0f81a62bb608756b127bfebfb21331)
    FetchContent_MakeAvailable(doctest)
    include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
endif()
