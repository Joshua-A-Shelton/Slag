cmake_minimum_required(VERSION 3.24)
project(Slag)
# make cache variables for install destinations
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)

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
            src/slag/BackEnd/Vulkan/VulkanShaderPipeline.cpp
            src/slag/BackEnd/Vulkan/VulkanShaderPipeline.h
            src/slag/BackEnd/Vulkan/VulkanDescriptorGroup.cpp
            src/slag/BackEnd/Vulkan/VulkanDescriptorGroup.h
            src/slag/BackEnd/Vulkan/VulkanDescriptorGroupCache.cpp
            src/slag/BackEnd/Vulkan/VulkanDescriptorGroupCache.h
            src/slag/BackEnd/Vulkan/VulkanDescriptorPool.cpp
            src/slag/BackEnd/Vulkan/VulkanDescriptorPool.h
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
            src/slag/BackEnd/DX12/DX12ShaderPipeline.cpp
            src/slag/BackEnd/DX12/DX12ShaderPipeline.h
            src/slag/BackEnd/DX12/DX12DescriptorGroup.cpp
            src/slag/BackEnd/DX12/DX12DescriptorGroup.h
    )
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
        src/slag/GpuMemoryBarriers.cpp
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
        src/slag/ShaderPipeline.cpp
        src/slag/ShaderPipeline.h
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
        src/slag/Attachment.h
        src/slag/QueryPool.cpp
        src/slag/QueryPool.h
        src/slag/DescriptorBundle.cpp
        src/slag/DescriptorBundle.h
        src/slag/FrameResources.cpp
        src/slag/FrameResources.h

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

    OPTION(SPIRV_REFLECT_EXECUTABLE "build spirv reflect executable" OFF)
    OPTION(SPIRV_REFLECT_EXAMPLES "build spirv reflect examples" OFF)
    OPTION(SPIRV_REFLECT_STATIC_LIB "build spirv reflect library" ON)
    add_subdirectory(third-party/spirv-reflect)

    target_include_directories(Slag PRIVATE ${Vulkan_INCLUDE_DIR} third-party/spirv-reflect)

    target_link_libraries(Slag ${Vulkan_LIBRARIES} vk-bootstrap GPUOpen::VulkanMemoryAllocator spirv-reflect-static)
endif()
if(SLAG_DX12_BACKEND)
    add_subdirectory(third-party/dx12-headers)
    add_subdirectory(third-party/d3dma)
    target_include_directories(Slag SYSTEM BEFORE PRIVATE DirectX-Headers)
    target_link_libraries(Slag DirectX-Headers D3D12MemoryAllocator)
    add_custom_command(TARGET Slag COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/third-party/D3D12 ${CMAKE_BINARY_DIR}/D3D12)
endif()


OPTION(BUILD_SLAG_TESTS "Build tests" OFF)
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
            test/tests/DescriptorGroupTest.cpp
            test/tests/ShaderTest.cpp
            test/third-party/LodePNG/lodepng.cpp
            test/third-party/LodePNG/lodepng.h
            test/tests/IntegrationTests.cpp
            test/utils/Window.cpp
            test/utils/Window.h
    )
    target_link_libraries(SlagTests GTest::gtest SDL2::SDL2 glm::glm Slag)
    target_include_directories(SlagTests PRIVATE src ${SDL2_LIBRARIES} third-party/stb test/third-party/LodePNG)
    include(GoogleTest)
    gtest_discover_tests(SlagTests)

    add_custom_command(TARGET SlagTests COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/test/resources ${CMAKE_BINARY_DIR}/resources)

    # SHADERS------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    set(SLAG_TESTS_SHADER_SOURCE_DIR test/resources/shaders)
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



endif()

target_include_directories(Slag PUBLIC src)