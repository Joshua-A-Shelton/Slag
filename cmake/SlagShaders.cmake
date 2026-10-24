# Compiles Slang shaders for tests, samples, and tools (ADR 0034). The library
# itself never compiles shaders.
#
#   slag_add_shader(<target> SOURCE <file.slang> ENTRY <name> STAGE <compute|vertex|pixel>)
#
# Writes <name-of-source>.<entry>.spv (Vulkan backend) and .dxil (D3D12
# backend) into ${CMAKE_CURRENT_BINARY_DIR}/shaders and makes <target> depend
# on them. SPIR-V is checked with spirv-val when it is available.

find_program(SLAG_SLANGC slangc)
find_program(SLAG_SPIRV_VAL spirv-val)

# Required for every Vulkan shader: descriptor heaps with one shared slot index
# space (ADRs 0029, 0034).
set(SLAG_SLANG_SPIRV_FLAGS
    -capability spvDescriptorHeapEXT
    -spirv-unified-descriptor-heap-stride
    -fvk-use-entrypoint-name)

function(slag_add_shader target)
    cmake_parse_arguments(ARG "" "SOURCE;ENTRY;STAGE" "" ${ARGN})
    if(NOT ARG_SOURCE OR NOT ARG_ENTRY OR NOT ARG_STAGE)
        message(FATAL_ERROR "slag_add_shader: SOURCE, ENTRY and STAGE are required")
    endif()
    if(NOT SLAG_SLANGC)
        message(FATAL_ERROR "slag_add_shader: slangc not found; set SLAG_SLANGC")
    endif()

    if(ARG_STAGE STREQUAL "compute")
        set(_dxil_profile cs_6_8)
    elseif(ARG_STAGE STREQUAL "vertex")
        set(_dxil_profile vs_6_8)
    elseif(ARG_STAGE STREQUAL "pixel")
        set(_dxil_profile ps_6_8)
    else()
        message(FATAL_ERROR "slag_add_shader: unknown STAGE '${ARG_STAGE}'")
    endif()

    get_filename_component(_source "${ARG_SOURCE}" ABSOLUTE)
    get_filename_component(_name "${ARG_SOURCE}" NAME_WE)
    set(_out_dir "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(_outputs)

    if(SLAG_BACKEND_VULKAN)
        set(_spv "${_out_dir}/${_name}.${ARG_ENTRY}.spv")
        set(_validate)
        if(SLAG_SPIRV_VAL)
            set(_validate COMMAND "${SLAG_SPIRV_VAL}" --target-env vulkan1.4 "${_spv}")
        endif()
        add_custom_command(OUTPUT "${_spv}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_out_dir}"
            COMMAND "${SLAG_SLANGC}" "${_source}" -target spirv -profile spirv_1_6
                -stage ${ARG_STAGE} -entry ${ARG_ENTRY} ${SLAG_SLANG_SPIRV_FLAGS} -o "${_spv}"
            ${_validate}
            DEPENDS "${_source}"
            COMMENT "Slang ${_name}.${ARG_ENTRY} -> SPIR-V"
            VERBATIM)
        list(APPEND _outputs "${_spv}")
    endif()

    if(SLAG_BACKEND_D3D12)
        set(_dxil "${_out_dir}/${_name}.${ARG_ENTRY}.dxil")
        add_custom_command(OUTPUT "${_dxil}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_out_dir}"
            COMMAND "${SLAG_SLANGC}" "${_source}" -target dxil -profile ${_dxil_profile}
                -entry ${ARG_ENTRY} -o "${_dxil}"
            DEPENDS "${_source}"
            COMMENT "Slang ${_name}.${ARG_ENTRY} -> DXIL"
            VERBATIM)
        list(APPEND _outputs "${_dxil}")
    endif()

    set(_shader_target "${target}_shader_${_name}_${ARG_ENTRY}")
    add_custom_target(${_shader_target} DEPENDS ${_outputs})
    add_dependencies(${target} ${_shader_target})
    get_target_property(_type ${target} TYPE)
    if(NOT _type STREQUAL "UTILITY")
        target_compile_definitions(${target} PRIVATE "SLAG_SHADER_DIR=\"${_out_dir}\"")
    endif()
endfunction()
