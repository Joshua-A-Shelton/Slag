cmake_minimum_required(VERSION 3.24)
project(Slag)

set(CMAKE_CXX_STANDARD 20)

SET(VULKAN_SOURCES
        src/slag/BackEnd/Vulkan/VulkanBuffer.h
        src/slag/BackEnd/Vulkan/VulkanCommandBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanCommandBuffer.h
        src/slag/BackEnd/Vulkan/VulkanComputeShader.cpp
        src/slag/BackEnd/Vulkan/VulkanComputeShader.h
        src/slag/BackEnd/Vulkan/VulkanCPUBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanCPUBuffer.h
        src/slag/BackEnd/Vulkan/VulkanCPUIndexBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanCPUIndexBuffer.h
        src/slag/BackEnd/Vulkan/VulkanCPUVertexBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanCPUVertexBuffer.h
        src/slag/BackEnd/Vulkan/VulkanDataBuffer.h
        src/slag/BackEnd/Vulkan/VulkanDescriptorAllocator.cpp
        src/slag/BackEnd/Vulkan/VulkanDescriptorAllocator.h
        src/slag/BackEnd/Vulkan/VulkanDescriptorSetLayoutCache.cpp
        src/slag/BackEnd/Vulkan/VulkanDescriptorSetLayoutCache.h
        src/slag/BackEnd/Vulkan/VulkanExtensions.cpp
        src/slag/BackEnd/Vulkan/VulkanExtensions.h
        src/slag/BackEnd/Vulkan/VulkanFrame.cpp
        src/slag/BackEnd/Vulkan/VulkanFrame.h
        src/slag/BackEnd/Vulkan/VulkanGPUBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanGPUBuffer.h
        src/slag/BackEnd/Vulkan/VulkanGPUDataBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanGPUDataBuffer.h
        src/slag/BackEnd/Vulkan/VulkanGPUIndexBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanGPUIndexBuffer.h
        src/slag/BackEnd/Vulkan/VulkanGPUVertexBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanGPUVertexBuffer.h
        src/slag/BackEnd/Vulkan/VulkanGraphicsCard.cpp
        src/slag/BackEnd/Vulkan/VulkanGraphicsCard.h
        src/slag/BackEnd/Vulkan/VulkanIndexBuffer.h
        src/slag/BackEnd/Vulkan/VulkanLib.cpp
        src/slag/BackEnd/Vulkan/VulkanLib.h
        src/slag/BackEnd/Vulkan/VulkanPushConstantRange.cpp
        src/slag/BackEnd/Vulkan/VulkanPushConstantRange.h
        src/slag/BackEnd/Vulkan/VulkanShader.cpp
        src/slag/BackEnd/Vulkan/VulkanShader.h
        src/slag/BackEnd/Vulkan/VulkanSwapchain.cpp
        src/slag/BackEnd/Vulkan/VulkanSwapchain.h
        src/slag/BackEnd/Vulkan/VulkanTexture.cpp
        src/slag/BackEnd/Vulkan/VulkanTexture.h
        src/slag/BackEnd/Vulkan/VulkanTextureSampler.cpp
        src/slag/BackEnd/Vulkan/VulkanTextureSampler.h
        src/slag/BackEnd/Vulkan/VulkanUniform.cpp
        src/slag/BackEnd/Vulkan/VulkanUniform.h
        src/slag/BackEnd/Vulkan/VulkanUniformBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanUniformBuffer.h
        src/slag/BackEnd/Vulkan/VulkanUniformDataSetFunctionProvider.cpp
        src/slag/BackEnd/Vulkan/VulkanUniformDataSetFunctionProvider.h
        src/slag/BackEnd/Vulkan/VulkanUniformLayoutInfo.cpp
        src/slag/BackEnd/Vulkan/VulkanUniformLayoutInfo.h
        src/slag/BackEnd/Vulkan/VulkanUniformSet.cpp
        src/slag/BackEnd/Vulkan/VulkanUniformSet.h
        src/slag/BackEnd/Vulkan/VulkanVertexBuffer.h
        src/slag/BackEnd/Vulkan/VulkanVirtualUniformBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanVirtualUniformBuffer.h
        src/slag/BackEnd/Vulkan/VulkanCPUDataBuffer.cpp
        src/slag/BackEnd/Vulkan/VulkanCPUDataBuffer.h)

SET(DX12_SOURCES)

OPTION(SLAG_VULKAN_BACKEND "Include Vulkan backend" ON)
OPTION(SLAG_DX12_BACKEND "Include DX12 backend" ON)
if(NOT WIN32)
    set(SLAG_DX12_BACKEND OFF)
endif()
set(SLAG_CONDITIONAL_SOURCES)
if(SLAG_VULKAN_BACKEND)
    find_package(Vulkan REQUIRED)
    add_definitions(-DSLAG_VULKAN_BACKEND=1)
    message("adding vulkan backend")
    list(APPEND SLAG_CONDITIONAL_SOURCES ${VULKAN_SOURCES})
endif()

if(SLAG_DX12_BACKEND)
    message("adding direct x 12 backend")
    file(GLOB_RECURSE DX12_SOURCES CONFIGURE_DEPENDS "src/slag/BackEnd/DX12/*.h" "src/slag/BackEnd/DX12/*.cpp")
    list(APPEND SLAG_CONDITIONAL_SOURCES ${DX12_SOURCES})
endif()
OPTION(BUILD_VULKAN_TEST_EXECUTABLE "Build test executable" ON)

add_library(Slag ${SLAG_CONDITIONAL_SOURCES}
        src/slag/UniformSet.h
        src/slag/Uniform.h
        src/slag/BackEnd/Resource.h
        src/slag/BackEnd/Resource.cpp
        src/slag/Frame.h
        src/slag/Frame.cpp
        src/slag/Swapchain.h
        src/slag/Swapchain.cpp
        src/slag/BackEnd/ResourceManager.cpp
        src/slag/BackEnd/ResourceManager.h
        src/slag/UniformDescriptor.h
        src/slag/SlagLib.cpp
        src/slag/SlagLib.h
        src/slag/CommandBuffer.h
        src/slag/Texture.h
        src/slag/PixelFormat.h
        src/slag/GraphicsCard.h
        src/slag/PlatformData.h
        src/slag/GPUMemoryBarriers.h
        src/slag/Shader.h
        src/slag/GPUTypes.h
        src/slag/UniformBuffer.h
        src/slag/UniformDescriptor.cpp
        src/slag/FramebufferDescription.cpp
        src/slag/FramebufferDescription.h
        src/slag/Shader.cpp
        src/slag/Buffer.h
        src/slag/Rectangle.h
        src/slag/Attachment.h
        src/slag/ClearValue.h
        src/slag/UniformSetData.h
        src/slag/UniformSetData.cpp
        src/slag/BackEnd/UniformDataSetFunctionProvider.h
        src/slag/BackEnd/UniformDataSetHandler.h
        src/slag/BackEnd/UniformDataSetHandler.cpp
        src/slag/UniformSetDataAllocator.h
        src/slag/TextureSampler.h
        src/slag/TextureSampler.cpp
        src/slag/Texture.cpp
        src/slag/PushConstantRange.h
        src/slag/VertexBuffer.cpp
        src/slag/VertexBuffer.h
        src/slag/IndexBuffer.cpp
        src/slag/IndexBuffer.h
        src/slag/VertexDescription.cpp
        src/slag/VertexDescription.h
        src/slag/Viewport.h
        src/slag/FrameResourceDescriptions.h
        src/slag/ComputeShader.cpp
        src/slag/ComputeShader.h
        src/slag/Color.cpp
        src/slag/Color.h
        src/slag/DataBuffer.cpp
        src/slag/DataBuffer.h
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

    target_link_libraries(Slag ${Vulkan_LIBRARIES} vk-bootstrap VulkanMemoryAllocator spirv-reflect-static)
endif()
if(SLAG_DX12_BACKEND)
    add_subdirectory(third-party/dx12-headers)
    target_link_libraries(Slag d3d12.lib dxgi.lib d3dcompiler.lib dxguid.lib)
endif()

if(BUILD_VULKAN_TEST_EXECUTABLE)
    message("building test executable")
    find_package(SDL2 CONFIG REQUIRED)
    add_executable(SlagTest test/Main.cpp)
    target_include_directories(SlagTest PRIVATE src ${SDL2_LIBRARIES})
    target_link_libraries(SlagTest Slag SDL2::SDL2)
    file(COPY test/resources DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endif()

target_include_directories(Slag PUBLIC src)
