cmake_minimum_required(VERSION 3.24)
project(Slag)

set(CMAKE_CXX_STANDARD 20)

OPTION(SLAG_VULKAN_BACKEND "Include Vulkan backend" ON)
OPTION(SLAG_DX12_BACKEND "Include DX12 backend" ON)
if(NOT WIN32)
    set(SLAG_DX12_BACKEND OFF)
endif()

find_package(Boost REQUIRED)

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
            src/slag/BackEnd/Vulkan/VulkanGPUMemoryReference.h
            src/slag/BackEnd/Vulkan/Extensions.cpp
            src/slag/BackEnd/Vulkan/Extensions.h
            src/slag/BackEnd/Vulkan/VulkanFrame.cpp
            src/slag/BackEnd/Vulkan/VulkanFrame.h
            src/slag/BackEnd/Vulkan/VulkanSwapchain.cpp
            src/slag/BackEnd/Vulkan/VulkanSwapchain.h
            src/slag/BackEnd/Vulkan/VulkanBuffer.cpp
            src/slag/BackEnd/Vulkan/VulkanBuffer.h
            src/slag/BackEnd/Vulkan/IVulkanCommandBuffer.cpp
            src/slag/BackEnd/Vulkan/IVulkanCommandBuffer.h
            src/slag/BackEnd/Vulkan/VulkanSampler.cpp
            src/slag/BackEnd/Vulkan/VulkanSampler.h
            src/slag/BackEnd/Vulkan/VulkanShader.cpp
            src/slag/BackEnd/Vulkan/VulkanShader.h
            src/slag/BackEnd/Vulkan/VulkanDescriptorGroup.cpp
            src/slag/BackEnd/Vulkan/VulkanDescriptorGroup.h
            src/slag/BackEnd/Vulkan/VulkanDescriptorGroupCache.cpp
            src/slag/BackEnd/Vulkan/VulkanDescriptorGroupCache.h
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
            src/slag/BackEnd/DX12/DX12Queue.h
            src/slag/BackEnd/DX12/IDX12CommandBuffer.cpp
            src/slag/BackEnd/DX12/IDX12CommandBuffer.h
            src/slag/BackEnd/DX12/DX12Swapchain.cpp
            src/slag/BackEnd/DX12/DX12Swapchain.h
            src/slag/BackEnd/DX12/DX12Frame.cpp
            src/slag/BackEnd/DX12/DX12Frame.h
            src/slag/BackEnd/DX12/DX12Buffer.cpp
            src/slag/BackEnd/DX12/DX12Buffer.h
            src/slag/BackEnd/DX12/DX12DescriptorPool.cpp
            src/slag/BackEnd/DX12/DX12DescriptorPool.h
            src/slag/BackEnd/DX12/DX12Sampler.cpp
            src/slag/BackEnd/DX12/DX12Sampler.h
            src/slag/BackEnd/DX12/DX12Shader.cpp
            src/slag/BackEnd/DX12/DX12Shader.h
            src/slag/BackEnd/DX12/DX12DescriptorGroup.cpp
            src/slag/BackEnd/DX12/DX12DescriptorGroup.h)
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
        src/slag/Swapchain.cpp
        src/slag/Swapchain.h
        src/slag/Frame.cpp
        src/slag/Frame.h
        src/slag/PlatformData.h
        src/slag/Clear.h
        src/slag/GpuMemoryBarriers.h
        src/slag/Buffer.cpp
        src/slag/Buffer.h
        src/slag/Rectangle.h
        src/slag/Resources/ResourceConsumer.h
        src/slag/Resources/ResourceConsumer.cpp
        src/slag/ICommandBuffer.h
        src/slag/DescriptorPool.cpp
        src/slag/DescriptorPool.h
        src/slag/Sampler.cpp
        src/slag/Sampler.h
        src/slag/Shader.cpp
        src/slag/Shader.h
        src/slag/Descriptor.cpp
        src/slag/Descriptor.h
        src/slag/DescriptorGroup.cpp
        src/slag/DescriptorGroup.h
        src/slag/GraphicsTypes.h
        src/slag/UniformBufferDescriptorLayout.cpp
        src/slag/UniformBufferDescriptorLayout.h
        src/slag/ShaderProperties.cpp
        src/slag/ShaderProperties.h
        src/slag/VertexDescription.cpp
        src/slag/VertexDescription.h
        src/slag/Operations.h
        src/slag/FrameBufferDescription.cpp
        src/slag/FrameBufferDescription.h
)
target_link_libraries(Slag ${Boost_LIBRARIES})
target_include_directories(Slag PRIVATE ${Boost_INCLUDE_DIRS} third-party/stb)

#needed for boost uuid
if(WIN32)
    target_link_libraries(Slag bcrypt)
endif()


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
    add_subdirectory(third-party/d3dma)
    target_include_directories(Slag SYSTEM BEFORE PUBLIC DirectX-Headers)
    target_link_libraries(Slag DirectX-Headers D3D12MemoryAllocator)
endif()


OPTION(BUILD_SLAG_TESTS "Build tests" ON)
if(BUILD_SLAG_TESTS)
    find_package(SDL2 REQUIRED)
    find_package(glm CONFIG REQUIRED)
    message("building tests")
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_definitions(-Dgtest_disable_pthreads=ON)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(SlagTests
            test/SlagTestsMain.cpp
            test/DX12Environment.cpp
            test/DX12Environment.h
            test/tests/CommandBufferTest.cpp
            test/VulkanEnvironment.cpp
            test/VulkanEnvironment.h
            test/tests/TextureTest.cpp
            test/tests/SwapchainTest.cpp
            test/tests/BufferTest.cpp
            test/tests/UniformDescriptorLayoutTest.cpp
            test/tests/UniformGroupTest.cpp
            test/tests/ShaderTest.cpp
    )
    target_link_libraries(SlagTests GTest::gtest SDL2::SDL2 glm::glm Slag)
    target_include_directories(SlagTests PRIVATE src ${SDL2_LIBRARIES} third-party/stb)
    include(GoogleTest)
    gtest_discover_tests(SlagTests)

    add_custom_command(TARGET SlagTests COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/test/resources ${CMAKE_CURRENT_BINARY_DIR}/resources)
endif()

target_include_directories(Slag PUBLIC src)
