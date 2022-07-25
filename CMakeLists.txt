cmake_minimum_required(VERSION 3.22)
project(SlagLib)

set(CMAKE_CXX_STANDARD 20)
find_package(Vulkan REQUIRED)
add_subdirectory(ThirdParty/vkbootstrap)
add_definitions(-DVMA_VULKAN_VERSION=1003000)

set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "don't build examples" FORCE)
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "don't build spirv-reflect executable" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "build static library" FORCE)
add_subdirectory(ThirdParty/spriv-reflect)

add_library(SlagLib src/SlagLib.cpp src/SlagLib.h src/GraphicsCard.cpp src/GraphicsCard.h src/Surface.cpp src/Surface.h src/BackEnd/VulkanBackEnd.h src/BackEnd/SwapChain.cpp src/BackEnd/SwapChain.h src/RenderAttachment.cpp src/RenderAttachment.h src/FrameBuffer.cpp src/FrameBuffer.h src/BackEnd/VulkanBackEnd.cpp src/Renderer.cpp src/Renderer.h src/Shader.cpp src/Shader.h)
target_include_directories(SlagLib PRIVATE ThirdParty/vkbootstrap/src ThirdParty/vma/include ThirdParty/spriv-reflect)

if(UNIX)
    find_package(X11)
    target_link_libraries(SlagLib ${X11_xcb_LIB})
endif(UNIX)
target_link_libraries(SlagLib vk-bootstrap vulkan spirv-reflect-static)

execute_process(COMMAND glslc -c ${CMAKE_SOURCE_DIR}/SlagTest/Shaders/triangle.vert -o ${CMAKE_SOURCE_DIR}/SlagTest/Shaders/triangle.vert.spv)
execute_process(COMMAND glslc -c ${CMAKE_SOURCE_DIR}/SlagTest/Shaders/triangle.frag -o ${CMAKE_SOURCE_DIR}/SlagTest/Shaders/triangle.frag.spv)

