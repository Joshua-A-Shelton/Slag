# Backend tests: need a real (or software) GPU, so they carry the "gpu" label.
# Hosted CI excludes them (ctest -LE gpu); the self-hosted GPU runner runs them.

add_executable(slag_backend_tests
    AgilityExports.cpp
    BackendTestMain.cpp
    BufferTests.cpp
    CommandTests.cpp
    ComputeTests.cpp
    DeviceTests.cpp
    InstanceTests.cpp)
target_link_libraries(slag_backend_tests PRIVATE slag::slag doctest::doctest)
doctest_discover_tests(slag_backend_tests PROPERTIES LABELS gpu)

# Shaders for the backend tests (SLAG_SHADER_DIR points at them).
slag_add_shader(slag_backend_tests SOURCE ../shaders/FillBuffer.slang ENTRY main STAGE compute)
if(SLAG_BACKEND_D3D12)
    target_compile_definitions(slag_backend_tests PRIVATE SLAG_TEST_AGILITY_SDK_VERSION=${SLAG_AGILITY_SDK_VERSION})
endif()
slag_copy_agility_runtime(slag_backend_tests)
