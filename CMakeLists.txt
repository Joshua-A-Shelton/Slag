cmake_minimum_required(VERSION 3.24)
project(Slag)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(FetchContent)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DSLAG_DEBUG=1)
    set(SLAG_DEBUG_BUILD ON)
endif()

OPTION(SLAG_VULKAN_BACKEND "Include Vulkan Renderer backend" ON)
OPTION(SLAG_DX12_BACKEND "Include DX12 Renderer backend" ON)

OPTION(SLAG_WIN32_BACKEND "Use Win32 Windowing Backend" ON)
OPTION(SLAG_X11_BACKEND "Use X11 Windowing Backend" ON)
OPTION(SLAG_WAYLAND_BACKEND "Use Wayland Windowing Backend" ON)

SET(USE_SPIRV_REFLECTION ON)

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
    SET(USE_SPIRV_REFLECTION ON)
    message("adding vulkan backend")
    SET(SLAG_VULKAN_SOURCES
            src/slag/backends/vulkan/VulkanBackend.cpp
            src/slag/backends/vulkan/VulkanBackend.h
            src/slag/backends/vulkan/core/VulkanCommandBuffer.cpp
            src/slag/backends/vulkan/core/VulkanCommandBuffer.h
            src/slag/backends/vulkan/core/VulkanTexture.cpp
            src/slag/backends/vulkan/core/VulkanTexture.h
            src/slag/backends/vulkan/core/VulkanBuffer.cpp
            src/slag/backends/vulkan/core/VulkanBuffer.h
            src/slag/backends/vulkan/core/IVulkanCommandBuffer.cpp
            src/slag/backends/vulkan/core/IVulkanCommandBuffer.h
            src/slag/backends/vulkan/core/VulkanGPUMemoryReference.h
            src/slag/backends/vulkan/core/VulkanGraphicsCard.cpp
            src/slag/backends/vulkan/core/VulkanGraphicsCard.h
            src/slag/backends/vulkan/core/VulkanQueue.cpp
            src/slag/backends/vulkan/core/VulkanQueue.h
            src/slag/backends/vulkan/core/VulkanSemaphore.cpp
            src/slag/backends/vulkan/core/VulkanSemaphore.h
            src/slag/backends/vulkan/core/VulkanDescriptorPool.cpp
            src/slag/backends/vulkan/core/VulkanDescriptorPool.h
            src/slag/backends/vulkan/core/VulkanShaderPipeline.cpp
            src/slag/backends/vulkan/core/VulkanShaderPipeline.h
            src/slag/backends/vulkan/core/VulkanDescriptorGroup.cpp
            src/slag/backends/vulkan/core/VulkanDescriptorGroup.h
            src/slag/backends/vulkan/core/VulkanDescriptorGroupCache.cpp
            src/slag/backends/vulkan/core/VulkanDescriptorGroupCache.h
            src/slag/backends/vulkan/core/VulkanSampler.cpp
            src/slag/backends/vulkan/core/VulkanSampler.h
            src/slag/backends/vulkan/core/VulkanSwapChain.cpp
            src/slag/backends/vulkan/core/VulkanSwapChain.h
            src/slag/backends/vulkan/core/VulkanFrame.cpp
            src/slag/backends/vulkan/core/VulkanFrame.h
            src/slag/backends/vulkan/core/VulkanBufferView.cpp
            src/slag/backends/vulkan/core/VulkanBufferView.h
            src/slag/backends/vulkan/core/VulkanDescriptorIndex.h)
endif()
if(SLAG_DX12_BACKEND)
    add_definitions(-DSLAG_DX12_BACKEND=1)
    SET(USE_DXIL_12_REFLECTION ON)
    message("adding direct x 12 backend")
    SET(SLAG_DX12_SOURCES
            src/slag/backends/dx12/DX12Backend.cpp
            src/slag/backends/dx12/DX12Backend.h
            src/slag/backends/dx12/core/DX12GraphicsCard.cpp
            src/slag/backends/dx12/core/DX12GraphicsCard.h
            src/slag/backends/dx12/core/DX12Queue.cpp
            src/slag/backends/dx12/core/DX12Queue.h
            src/slag/backends/dx12/core/IDX12CommandBuffer.cpp
            src/slag/backends/dx12/core/IDX12CommandBuffer.h
            src/slag/backends/dx12/core/DX12Buffer.cpp
            src/slag/backends/dx12/core/DX12Buffer.h
            src/slag/backends/dx12/core/DX12CommandBuffer.cpp
            src/slag/backends/dx12/core/DX12CommandBuffer.h
            src/slag/backends/dx12/core/DX12Semaphore.cpp
            src/slag/backends/dx12/core/DX12Semaphore.h
            src/slag/backends/dx12/core/DX12DescriptorPool.cpp
            src/slag/backends/dx12/core/DX12DescriptorPool.h
            src/slag/backends/dx12/core/DX12Texture.cpp
            src/slag/backends/dx12/core/DX12Texture.h
            src/slag/backends/dx12/core/DX12ShaderPipeline.cpp
            src/slag/backends/dx12/core/DX12ShaderPipeline.h
            src/slag/backends/dx12/core/DX12DescriptorGroup.cpp
            src/slag/backends/dx12/core/DX12DescriptorGroup.h
            src/slag/backends/dx12/core/DX12Sampler.cpp
            src/slag/backends/dx12/core/DX12Sampler.h
    )
endif()

if(USE_SPIRV_REFLECTION)
    add_definitions(-DUSE_SLAG_SPIRV_REFLECTION)
    SET(SLAG_SPIRV_REFLECT_SOURCES
            src/slag/spirv/SPIRVReflection.cpp
            src/slag/spirv/SPIRVReflection.h
            )
endif()

if(USE_DXIL_12_REFLECTION)
    add_definitions(-DUSE_SLAG_DXIL_12_REFLECTION)
    SET(SLAG_DXIL_12_REFLECT_SOURCES
            src/slag/dxil_12/DXIL12Reflection.cpp
            src/slag/dxil_12/DXIL12Reflection.h
    )
endif()

add_library(Slag ${SLAG_VULKAN_SOURCES} ${SLAG_DX12_SOURCES} ${SLAG_SPIRV_REFLECT_SOURCES} ${SLAG_DXIL_12_REFLECT_SOURCES}
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
        src/slag/core/Sampler.cpp
        src/slag/core/Sampler.h
        src/slag/core/Color.cpp
        src/slag/core/Color.h
        src/slag/core/Operations.h
        src/slag/core/Attachment.h
        src/slag/core/DescriptorBundle.cpp
        src/slag/core/DescriptorBundle.h
        src/slag/core/ShaderPipeline.cpp
        src/slag/core/ShaderPipeline.h
        src/slag/core/DescriptorGroup.h
        src/slag/core/Descriptor.cpp
        src/slag/core/Descriptor.h
        src/slag/core/GraphicsTypes.h
        src/slag/core/DescriptorPool.cpp
        src/slag/core/DescriptorPool.h
        src/slag/core/VertexDescription.cpp
        src/slag/core/VertexDescription.h
        src/slag/core/BufferView.cpp
        src/slag/core/BufferView.h
        src/slag/core/PixelFormatProperties.h

)
#link spirv if we need to
if(USE_SPIRV_REFLECTION)
    OPTION(SPIRV_REFLECT_EXECUTABLE "build spirv reflect executable" OFF)
    OPTION(SPIRV_REFLECT_EXAMPLES "build spirv reflect examples" OFF)
    OPTION(SPIRV_REFLECT_STATIC_LIB "build spirv reflect library" ON)
    FetchContent_Declare(
            SPIRV_REFLECT
            GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect
            #TODO: update this to vulkan 1.4, it's currently causing issues building on systems where the sdk isn't up to 1.4 yet, so we're waiting until it's widely available
            GIT_TAG vulkan-sdk-1.3.296.0
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(SPIRV_REFLECT)
    target_link_libraries(Slag PRIVATE spirv-reflect-static)
endif ()

if(USE_DXIL_12_REFLECTION)
    target_include_directories(Slag PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bin-libs/DXC/inc)

    find_library(DXCompiler_LIB NAMES dxcompiler PATHS ${CMAKE_CURRENT_SOURCE_DIR}/bin-libs/DXC/lib/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID})
    find_library(DXIL_LIB NAMES dxil PATHS ${CMAKE_CURRENT_SOURCE_DIR}/bin-libs/DXC/lib/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID})

    target_link_libraries(Slag PRIVATE ${DXCompiler_LIB} ${DXIL_LIB})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin-libs/DXC/bin/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/dxcompiler.dll ${CMAKE_BINARY_DIR}/dxcompiler.dll COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin-libs/DXC/bin/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/dxil.dll ${CMAKE_BINARY_DIR}/dxil.dll COPYONLY)
endif ()

if(SLAG_VULKAN_BACKEND)
    message("vulkan directory: ${Vulkan_INCLUDE_DIR}")
    FetchContent_Declare(
            VMA
            GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
            GIT_TAG v3.3.0
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(VMA)
    find_package(VMA REQUIRED)
    FetchContent_Declare(
            VKBOOTSTRAP
            GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
            #TODO: update this to vulkan 1.4, it's currently causing issues building on systems where the sdk isn't up to 1.4 yet, so we're waiting until it's widely available
            GIT_TAG v1.3.302
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(VKBOOTSTRAP)

    target_include_directories(Slag PUBLIC ${Vulkan_INCLUDE_DIR} ${vma_SOURCE_DIR})
    target_link_libraries(Slag PRIVATE ${Vulkan_LIBRARIES} GPUOpen::VulkanMemoryAllocator vk-bootstrap)
endif()

if(SLAG_DX12_BACKEND)
    FetchContent_Declare(
            DirectX-Headers
            GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers
            GIT_TAG v1.616.0
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(DirectX-Headers)
    find_package(DirectX-Headers REQUIRED)

    FetchContent_Declare(
            D3D12MemoryAllocator
            GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator
            GIT_TAG v3.0.1
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(D3D12MemoryAllocator)
    find_package(D3D12MemoryAllocator)

    #target_include_directories(Slag PUBLIC ${DirectX-Headers_SOURCE_DIR}/include/)
    target_link_libraries(Slag PRIVATE DirectX-Headers D3D12MemoryAllocator)


    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin-libs/D3D12/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/D3D12Core.dll ${CMAKE_BINARY_DIR}/D3D12/D3D12Core.dll COPYONLY)
    if(SLAG_DEBUG_BUILD)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin-libs/D3D12/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/D3D12Core.pdb ${CMAKE_BINARY_DIR}/D3D12/D3D12Core.pdb COPYONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin-libs/D3D12/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/d3d12SDKLayers.dll ${CMAKE_BINARY_DIR}/D3D12/d3d12SDKLayers.dll COPYONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin-libs/D3D12/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/d3d12SDKLayers.pdb ${CMAKE_BINARY_DIR}/D3D12/d3d12SDKLayers.pdb COPYONLY)
    endif()

endif()

target_include_directories(Slag PUBLIC src)

if(PROJECT_IS_TOP_LEVEL)
    fetchcontent_declare(
            SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL
            GIT_TAG release-2.32.8
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    fetchcontent_makeavailable(SDL2)
    fetchcontent_declare(
            GLM
            GIT_REPOSITORY https://github.com/g-truc/glm
            GIT_TAG 1.0.1
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    fetchcontent_makeavailable(GLM)
    fetchcontent_declare(
            STB_IMAGE
            GIT_REPOSITORY https://github.com/nothings/stb
            GIT_TAG origin/master
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(STB_IMAGE)
    find_package(STB_IMAGE)
    message("building tests")

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest
            GIT_TAG v1.17.0
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_definitions(-Dgtest_disable_pthreads=ON)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(SlagTests
            tests/SlagTestsMain.cpp
            tests/GraphicsAPIEnvironment.cpp
            tests/GraphicsAPIEnvironment.h
            tests/VulkanEnvironment.cpp
            tests/VulkanEnvironment.h
            tests/tests/CommandBuffer.cpp
            tests/Utilities.cpp
            tests/Utilities.h
            tests/tests/ShaderPipeline.cpp
            tests/tests/UniformBufferDescriptorLayout.cpp
            tests/third-party/LodePNG/lodepng.cpp
            tests/third-party/LodePNG/lodepng.h
            tests/tests/SwapChain.cpp
            tests/tests/Buffer.cpp
            tests/tests/Texture.cpp
            tests/tests/DescriptorPool.cpp
            tests/tests/DescriptorBundle.cpp
            tests/DX12Environment.cpp
            tests/DX12Environment.h
            tests/tests/GraphicsCard.cpp
    )

    target_link_libraries(SlagTests GTest::gtest SDL2::SDL2 glm::glm Slag)
    target_include_directories(SlagTests PRIVATE src ${SDL2_LIBRARIES} ${stb_image_SOURCE_DIR})
    include(GoogleTest)
    gtest_discover_tests(SlagTests)

    add_custom_command(TARGET SlagTests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/resources ${CMAKE_BINARY_DIR}/resources)

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


    add_executable(SlagCube
            examples/cube/Main.cpp
    )
    target_link_libraries(SlagCube SDL2::SDL2 glm::glm Slag)
    target_include_directories(SlagCube PRIVATE src ${SDL2_LIBRARIES} ${stb_image_SOURCE_DIR})

    add_dependencies(SlagCube slagTestShaders)

else()
    message("Skipping tests")
endif()