cmake_minimum_required(VERSION 3.24)
project(Slag)

set(CMAKE_CXX_STANDARD 20)





OPTION(SLAG_VULKAN_BACKEND "Include Vulkan backend" ON)
OPTION(SLAG_DX12_BACKEND "Include DX12 backend" ON)
if(NOT WIN32)
    set(SLAG_DX12_BACKEND OFF)
endif()

if(SLAG_VULKAN_BACKEND)
    find_package(Vulkan REQUIRED)
    add_definitions(-DSLAG_VULKAN_BACKEND=1)
    message("adding vulkan backend")
    SET(VULKAN_SOURCES
            src/slag/BackEnd/Vulkan/VulkanLib.cpp
            src/slag/BackEnd/Vulkan/VulkanLib.h
            src/slag/BackEnd/Vulkan/VulkanGraphicsCard.cpp
            src/slag/BackEnd/Vulkan/VulkanGraphicsCard.h
            src/slag/BackEnd/Vulkan/VulkanTexture.cpp
            src/slag/BackEnd/Vulkan/VulkanTexture.h
            src/slag/BackEnd/Vulkan/VulkanSemaphore.cpp
            src/slag/BackEnd/Vulkan/VulkanSemaphore.h
            src/slag/BackEnd/Vulkan/VulkanQueue.cpp
            src/slag/BackEnd/Vulkan/VulkanQueue.h
            src/slag/BackEnd/Vulkan/VulkanCommandBuffer.cpp
            src/slag/BackEnd/Vulkan/VulkanCommandBuffer.h
            src/slag/BackEnd/Vulkan/VulkanQueue.cpp
            src/slag/BackEnd/Vulkan/VulkanQueue.h
            src/slag/BackEnd/Vulkan/VulkanCommandBuffer.cpp
            src/slag/BackEnd/Vulkan/VulkanCommandBuffer.h
    )
endif()

if(SLAG_DX12_BACKEND)
    add_definitions(-DSLAG_DX12_BACKEND=1)
    message("adding direct x 12 backend")
    SET(DX12_SOURCES
            src/slag/BackEnd/DX12/DX12GraphicsCard.cpp
            src/slag/BackEnd/DX12/DX12GraphicsCard.h
            src/slag/BackEnd/DX12/DX12Lib.cpp
            src/slag/BackEnd/DX12/DX12Lib.h
            src/slag/BackEnd/DX12/DX12Texture.cpp
            src/slag/BackEnd/DX12/DX12Texture.h
            src/slag/BackEnd/DX12/DX12Semaphore.cpp
            src/slag/BackEnd/DX12/DX12Semaphore.h
            src/slag/BackEnd/DX12/DX12Queue.cpp
            src/slag/BackEnd/DX12/DX12Queue.h
            src/slag/BackEnd/DX12/DX12CommandBuffer.cpp
            src/slag/BackEnd/DX12/DX12CommandBuffer.h
            src/slag/BackEnd/DX12/DX12CommandBuffer.h
            src/slag/BackEnd/DX12/DX12Queue.cpp
            src/slag/BackEnd/DX12/DX12Queue.h)
endif()

add_library(Slag ${VULKAN_SOURCES} ${DX12_SOURCES}
        src/slag/Resources/Resource.cpp
        src/slag/Resources/Resource.h
        src/slag/CommandBuffer.cpp
        src/slag/CommandBuffer.h
        src/slag/Resources/ResourceManager.cpp
        src/slag/Resources/ResourceManager.h
        src/slag/SlagLib.cpp
        src/slag/SlagLib.h
        src/slag/Color.cpp
        src/slag/Color.h
        src/slag/Pixel.cpp
        src/slag/Pixel.h
        src/slag/GraphicsCard.cpp
        src/slag/GraphicsCard.h
        src/slag/GpuQueue.h
        src/slag/Texture.cpp
        src/slag/Texture.h
        src/slag/Semaphore.cpp
        src/slag/Semaphore.h
        src/slag/BackEnd/BackEndLib.h
        src/slag/BackEnd/BackEndLib.cpp
)
target_include_directories(Slag PRIVATE ${Boost_INCLUDE_DIRS} third-party/stb)


if(SLAG_VULKAN_BACKEND)
    add_subdirectory(third-party/vma)
    add_subdirectory(third-party/vk-bootstrap)

    SET(SPIRV_REFLECT_EXECUTABLE OFF)
    SET(SPIRV_REFLECT_EXAMPLES OFF)
    SET(SPIRV_REFLECT_STATIC_LIB ON)
    add_subdirectory(third-party/spirv-reflect)

    target_include_directories(Slag PRIVATE ${Vulkan_INCLUDE_DIR} third-party/spirv-reflect)

    target_link_libraries(Slag ${Vulkan_LIBRARIES} vk-bootstrap GPUOpen::VulkanMemoryAllocator spirv-reflect-static)
endif()
if(SLAG_DX12_BACKEND)
    add_subdirectory(third-party/dx12-headers)
    target_link_libraries(Slag d3d12.lib dxgi.lib d3dcompiler.lib dxguid.lib)
endif()


OPTION(BUILD_VULKAN_TESTS "Build tests" ON)
if(BUILD_VULKAN_TESTS)
    message("building tests")
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(SlagTests
            test/SlagTestsMain.cpp
            test/DX12Environment.cpp
            test/DX12Environment.h
            test/tests/CommandBufferTest.cc
            test/VulkanEnvironment.cpp
            test/VulkanEnvironment.h
    )
    target_link_libraries(SlagTests GTest::gtest Slag)
    target_include_directories(SlagTests PRIVATE src)
    include(GoogleTest)
    gtest_discover_tests(SlagTests)
endif()

target_include_directories(Slag PUBLIC src)
