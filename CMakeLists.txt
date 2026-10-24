cmake_minimum_required(VERSION 3.25)

project(Slag
    VERSION 0.1.0
    DESCRIPTION "Thin, low-level abstraction over D3D12 and Vulkan"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Options (CLAUDE.md §4.1, §8, ADR 0050)
# ---------------------------------------------------------------------------

if(WIN32)
    set(_slag_d3d12_default ON)
else()
    set(_slag_d3d12_default OFF)
endif()

option(SLAG_BACKEND_D3D12 "Build the D3D12 backend" ${_slag_d3d12_default})
option(SLAG_BACKEND_VULKAN "Build the Vulkan backend" ON)
option(SLAG_VALIDATION "Compile Slag validation in" OFF)
option(SLAG_DISABLE_LABELS "Strip debug labels" OFF)
option(SLAG_BUILD_TESTS "Build Slag tests" ${PROJECT_IS_TOP_LEVEL})
option(SLAG_BUILD_SAMPLES "Build Slag samples" ${PROJECT_IS_TOP_LEVEL})
set(SLAG_EXTERNAL_BACKEND_DIRS "" CACHE STRING
    "Directories containing out-of-tree (console) backends")

if(SLAG_BACKEND_D3D12 AND NOT WIN32)
    message(FATAL_ERROR "SLAG_BACKEND_D3D12 requires Windows")
endif()

if(NOT SLAG_BACKEND_D3D12 AND NOT SLAG_BACKEND_VULKAN AND NOT SLAG_EXTERNAL_BACKEND_DIRS)
    message(FATAL_ERROR "Enable at least one backend")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(SlagDependencies)
include(SlagShaders)

# ---------------------------------------------------------------------------
# Library
# ---------------------------------------------------------------------------

add_library(slag STATIC)
add_library(slag::slag ALIAS slag)

target_compile_features(slag PUBLIC cxx_std_20)

# slag/Version.h is generated from the project version (ADR 0070).
set(SLAG_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
configure_file(cmake/Version.h.in "${SLAG_GENERATED_INCLUDE_DIR}/slag/Version.h" @ONLY)

target_include_directories(slag PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${SLAG_GENERATED_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>)

target_include_directories(slag PRIVATE src)

target_sources(slag PRIVATE
    src/common/AttachmentView.cpp
    src/common/Buffer.cpp
    src/common/CommandBuffer.cpp
    src/common/Descriptors.cpp
    src/common/Formats.cpp
    src/common/GraphicsDevice.cpp
    src/common/HostAllocator.cpp
    src/common/Instance.cpp
    src/common/Logging.cpp
    src/common/MemoryBlock.cpp
    src/common/Platform.cpp
    src/common/Rendering.cpp
    src/common/SubAllocator.cpp
    src/common/Texture.cpp
    src/common/Tlsf.cpp)

target_compile_definitions(slag PUBLIC
    $<$<BOOL:${SLAG_BACKEND_D3D12}>:SLAG_BACKEND_D3D12>
    $<$<BOOL:${SLAG_BACKEND_VULKAN}>:SLAG_BACKEND_VULKAN>
    $<$<BOOL:${SLAG_VALIDATION}>:SLAG_VALIDATION>
    $<$<BOOL:${SLAG_DISABLE_LABELS}>:SLAG_DISABLE_LABELS>)

# No exceptions and no RTTI inside Slag (CLAUDE.md §8).
if(MSVC)
    target_compile_options(slag PRIVATE /W4 /permissive- /EHs-c- /GR-)
    target_compile_definitions(slag PRIVATE _HAS_EXCEPTIONS=0)
else()
    # Vulkan structs are initialized as {sType}; the rest is meant to be zero.
    target_compile_options(slag PRIVATE -Wall -Wextra -Wpedantic -Wno-missing-field-initializers
                                        -fno-exceptions -fno-rtti)
endif()

# ---------------------------------------------------------------------------
# Backends and registry (ADR 0065)
# ---------------------------------------------------------------------------

# Each backend appends its name to SLAG_BACKEND_NAMES and provides
# slag::slagBackend_<name>(). External backends do the same from their own
# CMakeLists.txt (set SLAG_BACKEND_NAMES in PARENT_SCOPE).
set(SLAG_BACKEND_NAMES)
set(_slag_builtin_namespaces)

if(SLAG_BACKEND_D3D12)
    target_link_libraries(slag PRIVATE slag_agility_sdk d3d12 dxgi)
    target_sources(slag PRIVATE
        src/d3d12/D3D12AttachmentView.cpp
        src/d3d12/D3D12Backend.cpp
        src/d3d12/D3D12Buffer.cpp
        src/d3d12/D3D12CommandBuffer.cpp
        src/d3d12/D3D12Descriptors.cpp
        src/d3d12/D3D12Device.cpp
        src/d3d12/D3D12Formats.cpp
        src/d3d12/D3D12Instance.cpp
        src/d3d12/D3D12Pipeline.cpp
        src/d3d12/D3D12Rendering.cpp
        src/d3d12/D3D12Semaphore.cpp
        src/d3d12/D3D12Texture.cpp)
    list(APPEND SLAG_BACKEND_NAMES d3d12)
    list(APPEND _slag_builtin_namespaces d3d12)
endif()

if(SLAG_BACKEND_VULKAN)
    target_link_libraries(slag PRIVATE Vulkan::Headers)
    target_sources(slag PRIVATE
        src/vulkan/VkAttachmentView.cpp
        src/vulkan/VkBackend.cpp
        src/vulkan/VkBuffer.cpp
        src/vulkan/VkCommandBuffer.cpp
        src/vulkan/VkDescriptors.cpp
        src/vulkan/VkDevice.cpp
        src/vulkan/VkFormats.cpp
        src/vulkan/VkInstance.cpp
        src/vulkan/VkLoader.cpp
        src/vulkan/VkMemory.cpp
        src/vulkan/VkPipeline.cpp
        src/vulkan/VkRendering.cpp
        src/vulkan/VkSemaphore.cpp
        src/vulkan/VkTexture.cpp)
    # Slag loads every Vulkan function itself (ADR 0061).
    target_compile_definitions(slag PRIVATE VK_NO_PROTOTYPES)
    if(NOT WIN32)
        target_link_libraries(slag PRIVATE ${CMAKE_DL_LIBS})
    endif()
    list(APPEND SLAG_BACKEND_NAMES vulkan)
    list(APPEND _slag_builtin_namespaces vk)
endif()

foreach(_slag_external_dir IN LISTS SLAG_EXTERNAL_BACKEND_DIRS)
    get_filename_component(_slag_external_name "${_slag_external_dir}" NAME)
    add_subdirectory("${_slag_external_dir}" "${CMAKE_CURRENT_BINARY_DIR}/external/${_slag_external_name}")
endforeach()

if(NOT SLAG_BACKEND_NAMES)
    message(FATAL_ERROR "No backend registered itself; check the enabled backends")
endif()

set(SLAG_BACKEND_DECLARATIONS)
set(SLAG_BACKEND_ENTRIES)
foreach(_name IN LISTS SLAG_BACKEND_NAMES)
    string(APPEND SLAG_BACKEND_DECLARATIONS "const BackendInfo* slagBackend_${_name}();\n")
    string(APPEND SLAG_BACKEND_ENTRIES "        slagBackend_${_name}(),\n")
endforeach()
configure_file(cmake/BackendRegistry.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/src/BackendRegistry.cpp" @ONLY)
target_sources(slag PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/src/BackendRegistry.cpp")

# Exactly one backend, and it's built in: dispatch directly (ADR 0065).
list(LENGTH SLAG_BACKEND_NAMES _slag_backend_count)
list(LENGTH _slag_builtin_namespaces _slag_builtin_count)
if(_slag_backend_count EQUAL 1 AND _slag_builtin_count EQUAL 1)
    target_compile_definitions(slag PRIVATE SLAG_DIRECT_BACKEND_NAMESPACE=${_slag_builtin_namespaces})
endif()

# ---------------------------------------------------------------------------
# Tests and samples
# ---------------------------------------------------------------------------

if(SLAG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(SLAG_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()
