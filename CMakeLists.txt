cmake_minimum_required(VERSION 3.24)
project(Slag)

set(CMAKE_CXX_STANDARD 20)

SET(VULKAN_SOURCES)
SET(DX12_SOURCES)

OPTION(SLAG_VULKAN_BACKEND "Include Vulkan backend" ON)
OPTION(SLAG_DX12_BACKEND "Include DX12 backend" ON)
if(!WIN32)
    set(SLAG_DX12_BACKEND OFF)
endif()

if(SLAG_VULKAN_BACKEND)
    find_package(Vulkan REQUIRED)
    add_definitions(-DSLAG_VULKAN_BACKEND=1)
    message("adding vulkan backend")
    file(GLOB_RECURSE VULKAN_SOURCES CONFIGURE_DEPENDS "src/slag/BackEnd/Vulkan/*.h" "src/slag/BackEnd/Vulkan/*.cpp")
endif()

if(SLAG_DX12_BACKEND)
    file(GLOB_RECURSE DX12_SOURCES CONFIGURE_DEPENDS "src/slag/BackEnd/DX12/*.h" "src/slag/BackEnd/DX12/*.cpp")
endif()

OPTION(BUILD_VULKAN_TEST_EXECUTABLE "Build test executable" OFF)


add_library(Slag ${VULKAN_SOURCES} ${DX12_SOURCES} src/slag/UniformSet.h src/slag/Uniform.h src/slag/BackEnd/Resource.h src/slag/BackEnd/Resource.cpp src/slag/Frame.h src/slag/Frame.h src/slag/Frame.cpp src/slag/Swapchain.h src/slag/Swapchain.cpp src/slag/BackEnd/ResourceManager.cpp src/slag/BackEnd/ResourceManager.h src/slag/UniformDescriptor.h src/slag/SlagLib.cpp src/slag/SlagLib.h src/slag/CommandBuffer.h src/slag/Texture.h src/slag/PixelFormat.h src/slag/GraphicsCard.h src/slag/PlatformData.h src/slag/GPUMemoryBarriers.h src/slag/Shader.h src/slag/GPUTypes.h src/slag/UniformBuffer.h src/slag/ResourceDescriptions.h src/slag/UniformDescriptor.cpp src/slag/FramebufferDescription.cpp src/slag/FramebufferDescription.h src/slag/Shader.cpp src/slag/Buffer.h src/slag/Buffer.cpp src/slag/Rectangle.h src/slag/Attachment.h src/slag/ClearValue.h src/slag/UniformSetData.h src/slag/UniformSetData.cpp src/slag/BackEnd/UniformDataSetFunctionProvider.h src/slag/BackEnd/UniformDataSetHandler.cpp src/slag/BackEnd/UniformDataSetHandler.h src/slag/UniformSetDataAllocator.h src/slag/TextureSampler.h src/slag/TextureSampler.cpp src/slag/Texture.cpp)
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

if(BUILD_VULKAN_TEST_EXECUTABLE)
    message("building test executable")
    find_package(SDL2 CONFIG REQUIRED)
    add_executable(SlagTest test/Main.cpp)
    target_include_directories(SlagTest PRIVATE src ${SDL2_LIBRARIES})
    target_link_libraries(SlagTest Slag SDL2::SDL2)
    file(COPY test/resources DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endif()

target_include_directories(Slag PUBLIC src)
