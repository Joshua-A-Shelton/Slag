cmake_minimum_required(VERSION 3.24)
project(Slag)
# make cache variables for install destinations
add_library(Slag
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
        src/slag/core/SLAG_ASSERT.h)

target_include_directories(Slag PUBLIC src)