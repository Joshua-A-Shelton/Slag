cmake_minimum_required(VERSION 3.24)
project(Slag)
set(CMAKE_CXX_STANDARD 20)
include(FetchContent)

OPTION(SLAG_DISCREET_TEXTURE_LAYOUTS "Switch the API of the library to toggle manual texture layout transitions" OFF)
if(SLAG_DISCREET_TEXTURE_LAYOUTS)
    add_definitions(-DSLAG_DISCREET_TEXTURE_LAYOUTS=1)
endif()

OPTION(SLAG_VULKAN_BACKEND "Include Vulkan Renderer backend" ON)
OPTION(SLAG_DX12_BACKEND "Include DX12 Renderer backend" ON)

OPTION(SLAG_WIN32_BACKEND "Use Win32 Windowing Backend" ON)
OPTION(SLAG_X11_BACKEND "Use X11 Windowing Backend" ON)
OPTION(SLAG_WAYLAND_BACKEND "Use Wayland Windowing Backend" ON)

if(WIN32)
    set(SLAG_X11_BACKEND OFF)
    set(SLAG_WAYLAND_BACKEND OFF)
    message("turned off x11 and wayland backends")
endif()
if(NOT WIN32)
    set(SLAG_DX12_BACKEND OFF)
    set(SLAG_WIN32_BACKEND OFF)
    message("turned off dx12 and win32 backends")
endif()
#enable backend macros
if(SLAG_WIN32_BACKEND)
    add_definitions(-DSLAG_WIN32_BACKEND=1)
    message("added win32 backend")
endif()
if(SLAG_X11_BACKEND)
    add_definitions(-DSLAG_X11_BACKEND=1)
    message("added x11 backend")
endif()
if(SLAG_WAYLAND_BACKEND)
    add_definitions(-DSLAG_WAYLAND_BACKEND=1)
    message("added wayland backend")
endif()

if(SLAG_VULKAN_BACKEND)
    find_package(Vulkan REQUIRED)
    add_definitions(-DSLAG_VULKAN_BACKEND=1)
    message("adding vulkan backend")
    SET(SLAG_VULKAN_SOURCES
            src/slag/backends/vulkan/VulkanBackend.cpp
            src/slag/backends/vulkan/VulkanBackend.h)
endif()
if(SLAG_DX12_BACKEND)
    add_definitions(-DSLAG_DX12_BACKEND=1)
    message("adding direct x 12 backend")
    SET(SLAG_DX12_SOURCES)
endif()

add_library(Slag ${SLAG_VULKAN_SOURCES} ${SLAG_DX12_SOURCES}
        src/slag/Slag.cpp
        src/slag/Slag.h
        src/slag/core/GraphicsCard.h
        src/slag/core/GPUQueue.h
        src/slag/core/CommandBuffer.cpp
        src/slag/core/CommandBuffer.h
        src/slag/core/Semaphore.cpp
        src/slag/core/Semaphore.h
        src/slag/core/Texture.cpp
        src/slag/core/Texture.h
        src/slag/core/Pixels.h
        src/slag/core/Pixels.cpp
        src/slag/utilities/SLAG_ASSERT.h
        src/slag/backends/Backend.h
        src/slag/core/ICommandBuffer.h
        src/slag/core/SwapChain.cpp
        src/slag/core/SwapChain.h
        src/slag/core/Frame.cpp
        src/slag/core/Frame.h
        src/slag/core/FrameResources.h
        src/slag/core/PlatformData.h
        src/slag/core/GPUBarriers.cpp
        src/slag/core/GPUBarriers.h
        src/slag/core/Buffer.cpp
        src/slag/core/Buffer.h
        src/slag/core/Clear.h
        src/slag/core/Dimensions.h
)

target_include_directories(Slag PUBLIC src)

if(PROJECT_IS_TOP_LEVEL)
    find_package(SDL2 REQUIRED)
    find_package(glm CONFIG REQUIRED)
    message("building tests")

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest
            GIT_TAG origin/main
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_definitions(-Dgtest_disable_pthreads=ON)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(SlagTests
            tests/SlagTestsMain.cpp
            tests/VulkanEnvironment.cpp
            tests/VulkanEnvironment.h
    )

    target_link_libraries(SlagTests GTest::gtest SDL2::SDL2 glm::glm Slag)
    target_include_directories(SlagTests PRIVATE src ${SDL2_LIBRARIES} third-party/stb test/third-party/LodePNG)
    include(GoogleTest)
    gtest_discover_tests(SlagTests)

    add_custom_command(TARGET SlagTests COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/resources ${CMAKE_BINARY_DIR}/resources)

    # SHADERS------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    set(SLAG_TESTS_SHADER_SOURCE_DIR tests/resources/shaders)
    set(SLAG_TESTS_SHADER_BINARY_DIR ${CMAKE_BINARY_DIR}/resources/shaders)

    file(GLOB SHADERS
            ${SLAG_TESTS_SHADER_SOURCE_DIR}/*.vert
            ${SLAG_TESTS_SHADER_SOURCE_DIR}/*.frag
            ${SLAG_TESTS_SHADER_SOURCE_DIR}/*.comp)

    add_custom_command(
            COMMAND
            ${CMAKE_COMMAND} -E make_directory ${SLAG_TESTS_SHADER_BINARY_DIR}
            OUTPUT ${SLAG_TESTS_SHADER_BINARY_DIR}
            COMMENT "Creating ${SLAG_TESTS_SHADER_BINARY_DIR}"
    )

    foreach(source IN LISTS SHADERS)
        get_filename_component(FILENAME ${source} NAME)
        add_custom_command(
                COMMAND
                glslc
                #      -MD -MF ${SHADER_BINARY_DIR}/${FILENAME}.d
                -o ${SLAG_TESTS_SHADER_BINARY_DIR}/${FILENAME}.spv
                ${source}
                OUTPUT ${SLAG_TESTS_SHADER_BINARY_DIR}/${FILENAME}.spv
                DEPENDS ${source} ${SLAG_TESTS_SHADER_BINARY_DIR}
                COMMENT "Compiling ${FILENAME}"
        )
        list(APPEND SLAG_TESTS_SPV_SHADERS ${SLAG_TESTS_SHADER_BINARY_DIR}/${FILENAME}.spv)
    endforeach()

    add_custom_target(slagTestShaders ALL DEPENDS ${SLAG_TESTS_SPV_SHADERS})

    add_dependencies(SlagTests slagTestShaders)

else()
    message("Skipping tests")
endif()