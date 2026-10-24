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

target_sources(slag PRIVATE
    src/common/Slag.cpp)

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
    target_compile_options(slag PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti)
endif()

# Backend sources are added here once they exist (src/d3d12, src/vulkan).
if(SLAG_BACKEND_D3D12)
    target_link_libraries(slag PRIVATE slag_agility_sdk)
endif()
if(SLAG_BACKEND_VULKAN)
    target_link_libraries(slag PRIVATE Vulkan::Headers)
endif()

foreach(_slag_external_dir IN LISTS SLAG_EXTERNAL_BACKEND_DIRS)
    get_filename_component(_slag_external_name "${_slag_external_dir}" NAME)
    add_subdirectory("${_slag_external_dir}" "${CMAKE_CURRENT_BINARY_DIR}/external/${_slag_external_name}")
endforeach()

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
