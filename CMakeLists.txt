cmake_minimum_required(VERSION 3.24)
project(Slag)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED)
add_library(Slag src/UniformSet.h src/UniformBuffer.h src/Resource.h src/Resource.cpp src/Frame.h src/Frame.h src/Frame.cpp src/Swapchain.h src/Swapchain.cpp src/BackEnd/ResourceManager.cpp src/BackEnd/ResourceManager.h src/Uniform.h src/SlagLib.cpp src/SlagLib.h src/CommandBuffer.h src/Texture.h src/PixelFormat.h)
target_include_directories(Slag PRIVATE ${Boost_INCLUDE_DIRS})
