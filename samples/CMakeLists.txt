# Samples double as smoke tests (CLAUDE.md §12): triangle, textured_quad,
# compute, bindless, multi_queue. Each takes --frames N for a scripted run;
# the "sample" CTest label runs them where a window can be shown.

add_library(slag_sample_common STATIC common/SampleApp.cpp)
target_include_directories(slag_sample_common PUBLIC common)
target_link_libraries(slag_sample_common PUBLIC slag::slag SDL3::SDL3-static)
target_compile_features(slag_sample_common PUBLIC cxx_std_20)
if(SLAG_BACKEND_D3D12)
    target_compile_definitions(slag_sample_common PRIVATE SAMPLE_AGILITY_SDK_VERSION=${SLAG_AGILITY_SDK_VERSION})
endif()
if(UNIX AND NOT APPLE)
    # XCB windows come from SDL's X11 display through Xlib-xcb.
    find_library(SLAG_X11_XCB_LIBRARY X11-xcb)
    if(SLAG_X11_XCB_LIBRARY)
        target_sources(slag_sample_common PRIVATE common/XcbConnection.cpp)
        target_compile_definitions(slag_sample_common PRIVATE SAMPLE_HAS_XCB)
        target_link_libraries(slag_sample_common PRIVATE ${SLAG_X11_XCB_LIBRARY})
    endif()
endif()

set(SLAG_SAMPLE_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
target_compile_definitions(slag_sample_common PUBLIC "SAMPLE_SHADER_DIR=\"${SLAG_SAMPLE_SHADER_DIR}\"")

function(slag_add_sample name)
    cmake_parse_arguments(ARG "NO_TEST" "" "SHADERS;COMPUTE_SHADERS" ${ARGN})
    add_executable(${name} ${name}/main.cpp)
    target_link_libraries(${name} PRIVATE slag_sample_common)
    foreach(_shader IN LISTS ARG_SHADERS)
        slag_add_shader(${name} SOURCE shaders/${_shader}.slang ENTRY vertexMain STAGE vertex)
        slag_add_shader(${name} SOURCE shaders/${_shader}.slang ENTRY pixelMain STAGE pixel)
    endforeach()
    foreach(_shader IN LISTS ARG_COMPUTE_SHADERS)
        slag_add_shader(${name} SOURCE shaders/${_shader}.slang ENTRY computeMain STAGE compute)
    endforeach()
    slag_copy_agility_runtime(${name})
    if(SLAG_BUILD_TESTS AND NOT ARG_NO_TEST)
        add_test(NAME sample_${name} COMMAND ${name} --frames 120)
        set_tests_properties(sample_${name} PROPERTIES LABELS "sample;gpu")
    endif()
endfunction()

slag_add_sample(triangle SHADERS Triangle)
slag_add_sample(textured_quad SHADERS TexturedQuad)
slag_add_sample(compute SHADERS FullScreenTexture COMPUTE_SHADERS Plasma)
slag_add_sample(bindless SHADERS Bindless)
# Loses the device on purpose (ADR 0014), so it is run by hand, never by CTest.
slag_add_sample(crash NO_TEST COMPUTE_SHADERS Crash)
slag_add_sample(multi_queue SHADERS Particles FullScreenTexture COMPUTE_SHADERS ParticleUpdate)
