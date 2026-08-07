cmake_minimum_required(VERSION 3.24)
project(Slag)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(FetchContent)

add_library(Slag)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Slag INTERFACE SLAG_DEBUG=1)
    add_definitions(-DSLAG_DEBUG=1)
    set(SLAG_DEBUG_BUILD ON)
endif()

OPTION(SLAG_VULKAN_BACKEND "Include Vulkan Renderer backend" ON)
OPTION(SLAG_DX12_BACKEND "Include DX12 Renderer backend" ON)

OPTION(SLAG_WIN32_BACKEND "Use Win32 Windowing Backend" ON)
OPTION(SLAG_X11_BACKEND "Use X11 Windowing Backend" ON)
OPTION(SLAG_WAYLAND_BACKEND "Use Wayland Windowing Backend" ON)

OPTION(SLAG_BUILD_TESTS "Build Slag Tests" ON)
OPTION(SLAG_BUILD_CUBE "Build Slag Example Cube" ON)
OPTION(SLAG_BUILD_MS_CUBE "Build Multi Sample Example Cube" ON)

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
    target_compile_definitions(Slag INTERFACE SLAG_WIN32_BACKEND=1)
    add_definitions(-DSLAG_WIN32_BACKEND=1)
    message("added win32 backend")
endif()
if(SLAG_X11_BACKEND)
    target_compile_definitions(Slag INTERFACE SLAG_X11_BACKEND=1)
    add_definitions(-DSLAG_X11_BACKEND=1)
    message("added x11 backend")
endif()
if(SLAG_WAYLAND_BACKEND)
    target_compile_definitions(Slag INTERFACE SLAG_WAYLAND_BACKEND=1)
    add_definitions(-DSLAG_WAYLAND_BACKEND=1)
    message("added wayland backend")
endif()

if(SLAG_VULKAN_BACKEND)
    find_package(Vulkan 1.4.341 REQUIRED)
    target_compile_definitions(Slag INTERFACE SLAG_VULKAN_BACKEND=1)
    add_definitions(-DSLAG_VULKAN_BACKEND=1)
    message("adding vulkan backend")
    SET(SLAG_VULKAN_SOURCES
            src/slag/backends/vulkan/VulkanBackend.cpp
            src/slag/backends/vulkan/VulkanBackend.h
            src/slag/backends/vulkan/VulkanGraphicsCard.cpp
            src/slag/backends/vulkan/VulkanGraphicsCard.h
            src/slag/backends/vulkan/VulkanBuffer.cpp
            src/slag/backends/vulkan/VulkanBuffer.h
            src/slag/backends/vulkan/VulkanSemaphore.cpp
            src/slag/backends/vulkan/VulkanSemaphore.h
            src/slag/backends/vulkan/VulkanCommandBuffer.cpp
            src/slag/backends/vulkan/VulkanCommandBuffer.h
            src/slag/backends/vulkan/IVulkanCommandBuffer.cpp
            src/slag/backends/vulkan/IVulkanCommandBuffer.h
            src/slag/backends/vulkan/VulkanSubmissionQueue.cpp
            src/slag/backends/vulkan/VulkanSubmissionQueue.h
            src/slag/backends/vulkan/VulkanTexture.cpp
            src/slag/backends/vulkan/VulkanTexture.h
            src/slag/backends/vulkan/VulkanShaderPipeline.cpp
            src/slag/backends/vulkan/VulkanShaderPipeline.h
            src/slag/backends/vulkan/VulkanSampler.cpp
            src/slag/backends/vulkan/VulkanSampler.h
            src/slag/backends/vulkan/VulkanSamplerDescriptorHeap.cpp
            src/slag/backends/vulkan/VulkanSamplerDescriptorHeap.h
            src/slag/backends/vulkan/VulkanResourceDescriptorHeap.cpp
            src/slag/backends/vulkan/VulkanResourceDescriptorHeap.h
            src/slag/backends/vulkan/VulkanSwapChain.cpp
            src/slag/backends/vulkan/VulkanSwapChain.h
            src/slag/backends/vulkan/VulkanFrame.cpp
            src/slag/backends/vulkan/VulkanFrame.h
    )
endif()
if(SLAG_DX12_BACKEND)
    target_compile_definitions(Slag INTERFACE SLAG_DX12_BACKEND=1)
    add_definitions(-DSLAG_DX12_BACKEND=1)
    message("adding direct x 12 backend")
    SET(SLAG_DX12_SOURCES
            src/slag/backends/dx12/DX12Backend.cpp
            src/slag/backends/dx12/DX12Backend.h
            src/slag/backends/dx12/DX12GraphicsCard.cpp
            src/slag/backends/dx12/DX12GraphicsCard.h
            src/slag/backends/dx12/DX12Buffer.cpp
            src/slag/backends/dx12/DX12Buffer.h
            src/slag/backends/dx12/DX12Semaphore.cpp
            src/slag/backends/dx12/DX12Semaphore.h
            src/slag/backends/dx12/DX12SubmissionQueue.cpp
            src/slag/backends/dx12/DX12SubmissionQueue.h
            src/slag/backends/dx12/IDX12CommandBuffer.cpp
            src/slag/backends/dx12/IDX12CommandBuffer.h
            src/slag/backends/dx12/DX12CommandBuffer.cpp
            src/slag/backends/dx12/DX12CommandBuffer.h
            src/slag/backends/dx12/DX12Texture.cpp
            src/slag/backends/dx12/DX12Texture.h
            src/slag/backends/dx12/DX12ShaderPipeline.cpp
            src/slag/backends/dx12/DX12ShaderPipeline.h
            src/slag/backends/dx12/DX12Sampler.cpp
            src/slag/backends/dx12/DX12Sampler.h
            src/slag/backends/dx12/DX12ResourceDescriptorHeap.cpp
            src/slag/backends/dx12/DX12ResourceDescriptorHeap.h
            src/slag/backends/dx12/DX12SamplerDescriptorHeap.cpp
            src/slag/backends/dx12/DX12SamplerDescriptorHeap.h
            src/slag/core/Frame.h
            src/slag/backends/dx12/DX12SwapChain.cpp
            src/slag/backends/dx12/DX12SwapChain.h
            src/slag/backends/dx12/DX12Frame.cpp
            src/slag/backends/dx12/DX12Frame.h
    )
endif()

SET(SLAG_SOURCES ${SLAG_VULKAN_SOURCES} ${SLAG_DX12_SOURCES} ${SLAG_SPIRV_REFLECT_SOURCES} ${SLAG_DXIL_12_REFLECT_SOURCES}
        src/slag/Slag.cpp
        src/slag/Slag.h
        src/slag/core/GraphicsCard.cpp
        src/slag/core/GraphicsCard.h
        src/slag/core/Buffer.cpp
        src/slag/core/Buffer.h
        src/slag/core/IBackend.cpp
        src/slag/core/IBackend.h
        src/slag/exceptions/NotImplemented.h
        src/slag/core/Texture.cpp
        src/slag/core/Texture.h
        src/slag/core/Pixels.cpp
        src/slag/core/Pixels.h
        src/slag/core/ICommandBuffer.h
        src/slag/core/CommandBuffer.h
        src/slag/core/Dimensions.h
        src/slag/core/SubmissionQueue.h
        src/slag/core/Semaphore.h
        src/slag/utilities/SLAG_ASSERT.h
        src/slag/exceptions/ResourceCreationError.h
        src/slag/core/PixelFormatProperties.h
        src/slag/core/Defragmentation.h
        src/slag/core/Barriers.h
        src/slag/core/ShaderPipeline.h
        src/slag/core/ShaderCode.h
        src/slag/core/GraphicsTypes.h
        src/slag/exceptions/InvalidShaderCodeError.h
        src/slag/core/Operations.h
        src/slag/core/Color.cpp
        src/slag/core/Color.h
        src/slag/core/VertexDescription.cpp
        src/slag/core/VertexDescription.h
        src/slag/exceptions/InvalidShaderVertexBindingError.h
        src/slag/exceptions/IncompatibleShaderBindGroupsError.h
        src/slag/core/Clear.h
        src/slag/core/Attachment.h
        src/slag/core/Sampler.h
        src/slag/exceptions/PipelineInputMappingSizeExceeded.h
        src/slag/core/SamplerDescriptorHeap.h
        src/slag/core/ResourceDescriptorHeap.h
        src/slag/core/SwapChain.h
        src/slag/core/Frame.h
        src/slag/core/PlatformData.h
        src/slag/exceptions/InvalidSwapChainOperation.h
)

target_sources(Slag PRIVATE ${SLAG_SOURCES})

if(SLAG_VULKAN_BACKEND)
    message("vulkan directory: ${Vulkan_INCLUDE_DIR}")
    find_package(VulkanMemoryAllocator QUIET)
    if(NOT VulkanMemoryAllocator_FOUND)
        FetchContent_Declare(
                VulkanMemoryAllocator
                GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
                GIT_TAG v3.3.0
                GIT_SHALLOW TRUE
                OVERRIDE_FIND_PACKAGE
        )
        FetchContent_MakeAvailable(VulkanMemoryAllocator)
    endif ()
    find_package(VulkanMemoryAllocator REQUIRED)

    find_package(vk-bootstrap QUIET)
    if(NOT vk-bootstrap_FOUND)
        FetchContent_Declare(
                vk-bootstrap
                GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
                GIT_TAG v1.4.341
                GIT_SHALLOW TRUE
                OVERRIDE_FIND_PACKAGE
        )
        FetchContent_MakeAvailable(vk-bootstrap)
    endif ()

    target_include_directories(Slag PRIVATE ${Vulkan_INCLUDE_DIR})
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
add_library(slag::Slag ALIAS Slag)

if(SLAG_BUILD_TESTS OR SLAG_BUILD_CUBE)
    find_package(SDL3 QUIET)
    if(NOT SDL3_FOUND)
        fetchcontent_declare(
                SDL3
                GIT_REPOSITORY https://github.com/libsdl-org/SDL
                GIT_TAG release-3.4.0
                GIT_SHALLOW TRUE
                OVERRIDE_FIND_PACKAGE
        )
        FetchContent_MakeAvailable(SDL3)
        find_package(SDL3 REQUIRED)
    endif ()

    find_package(glm)
    if(NOT glm_FOUND)
        fetchcontent_declare(
                GLM
                GIT_REPOSITORY https://github.com/g-truc/glm
                GIT_TAG 1.0.1
                GIT_SHALLOW TRUE
                OVERRIDE_FIND_PACKAGE
        )
        FetchContent_MakeAvailable(glm)
        find_package(glm REQUIRED)
    endif ()

    fetchcontent_declare(
            STB_IMAGE
            GIT_REPOSITORY https://github.com/nothings/stb
            GIT_TAG origin/master
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(STB_IMAGE)
    find_package(STB_IMAGE)

endif ()

if(SLAG_BUILD_TESTS)
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
            tests/third-party/LodePNG/lodepng.cpp
            tests/third-party/LodePNG/lodepng.h
            tests/SlagTestsMain.cpp
            tests/tests/GraphicsCard.cpp
            tests/tests/Buffer.cpp
            tests/utilities/GeneralUtilities.h
            tests/tests/Semaphore.cpp
            tests/tests/CommandBuffer.cpp
            tests/tests/SubmissionQueue.cpp
            tests/tests/Texture.cpp
            tests/tests/ShaderCode.cpp
            tests/utilities/PlatformUtilities.cpp
            tests/utilities/PlatformUtilities.h
            tests/utilities/SlagUtilities.cpp
            tests/utilities/SlagUtilities.h
    )

    target_link_libraries(SlagTests GTest::gtest SDL3::SDL3 glm::glm slag::Slag)
    target_include_directories(SlagTests PRIVATE ${stb_image_SOURCE_DIR})

    file(GLOB TEST_VERT_FRAG_SHADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/tests/shaders/src/VertexAndFragment/*.slang")

    foreach (SHADER ${TEST_VERT_FRAG_SHADERS})
        get_filename_component(FILENAME ${SHADER} NAME_WE)
        set(OUTPUT_FILE_RAW "${CMAKE_CURRENT_SOURCE_DIR}/resources/tests/shaders/compiled/${FILENAME}")
        add_custom_command(
                OUTPUT "${OUTPUT_FILE_RAW}.vertex.spv"
                COMMAND slangc ${SHADER} -target spirv -capability spvDescriptorHeapEXT -spirv-unified-descriptor-heap-stride -entry vertexMain -o "${OUTPUT_FILE_RAW}.vertex.spv"
                DEPENDS ${SHADER}
                COMMENT "Compiling ${FILENAME} to spirv vertex"
        )
        add_custom_command(
                OUTPUT "${OUTPUT_FILE_RAW}.fragment.spv"
                COMMAND slangc ${SHADER} -target spirv -capability spvDescriptorHeapEXT -spirv-unified-descriptor-heap-stride -entry fragmentMain -o "${OUTPUT_FILE_RAW}.fragment.spv"
                DEPENDS ${SHADER}
                COMMENT "Compiling ${FILENAME} to spirv fragment"
        )
        if(WIN32)
            add_custom_command(
                    OUTPUT "${OUTPUT_FILE_RAW}.vertex.dxil"
                    COMMAND slangc ${SHADER} -target dxil -profile sm_6_6 -entry vertexMain -o "${OUTPUT_FILE_RAW}.vertex.dxil"
                    DEPENDS ${SHADER}
                    COMMENT "Compiling ${FILENAME} to dxil vertex"
            )
            add_custom_command(
                    OUTPUT "${OUTPUT_FILE_RAW}.fragment.dxil"
                    COMMAND slangc ${SHADER} -target dxil -profile sm_6_6 -entry fragmentMain -o "${OUTPUT_FILE_RAW}.fragment.dxil"
                    DEPENDS ${SHADER}
                    COMMENT "Compiling ${FILENAME} to dxil fragment"
            )
        endif ()
        list(APPEND COMPILED_TEST_SHADER_FILES ${OUTPUT_FILE_RAW}.vertex.spv ${OUTPUT_FILE_RAW}.fragment.spv ${OUTPUT_FILE_RAW}.vertex.dxil ${OUTPUT_FILE_RAW}.fragment.dxil)
    endforeach ()

    file(GLOB TEST_COMPUTE_SHADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/tests/shaders/src/Compute/*.slang")

    foreach (SHADER ${TEST_COMPUTE_SHADERS})
        get_filename_component(FILENAME ${SHADER} NAME_WE)
        set(OUTPUT_FILE_RAW "${CMAKE_CURRENT_SOURCE_DIR}/resources/tests/shaders/compiled/${FILENAME}")
        add_custom_command(
                OUTPUT "${OUTPUT_FILE_RAW}.compute.spv"
                COMMAND slangc ${SHADER} -target spirv -capability spvDescriptorHeapEXT -spirv-unified-descriptor-heap-stride -entry computeMain -o "${OUTPUT_FILE_RAW}.compute.spv"
                DEPENDS ${SHADER}
                COMMENT "Compiling ${FILENAME} to spirv compute"
        )
        if(WIN32)
            add_custom_command(
                    OUTPUT "${OUTPUT_FILE_RAW}.compute.dxil"
                    COMMAND slangc ${SHADER} -target dxil -profile sm_6_6 -entry computeMain -o "${OUTPUT_FILE_RAW}.compute.dxil"
                    DEPENDS ${SHADER}
                    COMMENT "Compiling ${FILENAME} to dxil compute"
            )
        endif ()
        list(APPEND COMPILED_TEST_SHADER_FILES ${OUTPUT_FILE_RAW}.compute.spv ${OUTPUT_FILE_RAW}.compute.dxil)
    endforeach ()

    add_custom_target(CompileTestShaders ALL DEPENDS ${COMPILED_TEST_SHADER_FILES} ${TEST_COMPUTE_SHADERS})

    add_custom_command(
            TARGET SlagTests
            POST_BUILD
            COMMAND  ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/tests
            ${CMAKE_CURRENT_BINARY_DIR}/resources/tests
            COMMENT "Copying test resource files"
    )

    add_dependencies(SlagTests CompileTestShaders)
endif ()

#Examples

file(GLOB EXAMPLE_VERT_FRAG_SHADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/examples/shaders/src/VertexAndFragment/*.slang")

foreach (SHADER ${EXAMPLE_VERT_FRAG_SHADERS})
    get_filename_component(FILENAME ${SHADER} NAME_WE)
    set(OUTPUT_FILE_RAW "${CMAKE_CURRENT_SOURCE_DIR}/resources/examples/shaders/compiled/${FILENAME}")
    add_custom_command(
            OUTPUT "${OUTPUT_FILE_RAW}.vertex.spv"
            COMMAND slangc ${SHADER} -target spirv -capability spvDescriptorHeapEXT -spirv-unified-descriptor-heap-stride -entry vertexMain -o "${OUTPUT_FILE_RAW}.vertex.spv"
            DEPENDS ${SHADER}
            COMMENT "Compiling ${FILENAME} to spirv vertex"
    )
    add_custom_command(
            OUTPUT "${OUTPUT_FILE_RAW}.fragment.spv"
            COMMAND slangc ${SHADER} -target spirv -capability spvDescriptorHeapEXT -spirv-unified-descriptor-heap-stride -entry fragmentMain -o "${OUTPUT_FILE_RAW}.fragment.spv"
            DEPENDS ${SHADER}
            COMMENT "Compiling ${FILENAME} to spirv fragment"
    )
    if(WIN32)
        add_custom_command(
                OUTPUT "${OUTPUT_FILE_RAW}.vertex.dxil"
                COMMAND slangc ${SHADER} -target dxil -profile sm_6_6 -entry vertexMain -o "${OUTPUT_FILE_RAW}.vertex.dxil"
                DEPENDS ${SHADER}
                COMMENT "Compiling ${FILENAME} to dxil vertex"
        )
        add_custom_command(
                OUTPUT "${OUTPUT_FILE_RAW}.fragment.dxil"
                COMMAND slangc ${SHADER} -target dxil -profile sm_6_6 -entry fragmentMain -o "${OUTPUT_FILE_RAW}.fragment.dxil"
                DEPENDS ${SHADER}
                COMMENT "Compiling ${FILENAME} to dxil fragment"
        )
        list(APPEND COMPILED_EXAMPLE_SHADER_FILES ${OUTPUT_FILE_RAW}.vertex.dxil ${OUTPUT_FILE_RAW}.fragment.dxil)
    endif ()
    list(APPEND COMPILED_EXAMPLE_SHADER_FILES ${OUTPUT_FILE_RAW}.vertex.spv ${OUTPUT_FILE_RAW}.fragment.spv)
endforeach ()

add_custom_target(CompileExampleShaders ALL DEPENDS ${COMPILED_EXAMPLE_SHADER_FILES})
add_custom_command(
        TARGET CompileExampleShaders
        POST_BUILD
        COMMAND  ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/examples/
        ${CMAKE_CURRENT_BINARY_DIR}/resources/examples/
        COMMENT "Copying test resource files"
)

if(SLAG_BUILD_CUBE)
    add_executable(SlagCube
            examples/cube/Main.cpp
    )
    target_link_libraries(SlagCube SDL3::SDL3 glm::glm slag::Slag)
    target_include_directories(SlagCube PRIVATE ${stb_image_SOURCE_DIR})

    add_dependencies(SlagCube CompileExampleShaders)
endif()
if(SLAG_BUILD_MS_CUBE)
    add_executable(SlagMSCube
            "examples/multi-sample cube/Main.cpp"
    )
    target_link_libraries(SlagMSCube SDL3::SDL3 glm::glm slag::Slag)
    target_include_directories(SlagMSCube PRIVATE ${stb_image_SOURCE_DIR})

    add_dependencies(SlagMSCube CompileExampleShaders)
endif ()