add_subdirectory(unit)
add_subdirectory(backend)

# Test shaders, compiled for every enabled backend.
add_custom_target(slag_test_shaders ALL)
slag_add_shader(slag_test_shaders SOURCE shaders/FillBuffer.slang ENTRY main STAGE compute)
