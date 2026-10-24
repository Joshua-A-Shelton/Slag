# Backend tests: need a real (or software) GPU, so they carry the "gpu" label.
# Hosted CI excludes them (ctest -LE gpu); the self-hosted GPU runner runs them.

add_executable(slag_backend_tests
    AgilityExports.cpp
    BackendTestMain.cpp
    BufferTests.cpp
    CommandTests.cpp
    ComputeTests.cpp
    CopyTests.cpp
    DefragmentationTests.cpp
    DescriptorTests.cpp
    DeviceTests.cpp
    InstanceTests.cpp
    MemoryTests.cpp
    NativeTests.cpp
    QueryTests.cpp
    ImageCheck.cpp
    RenderTests.cpp
    SceneTests.cpp
    TextureTests.cpp
    UtilTests.cpp)
target_link_libraries(slag_backend_tests PRIVATE slag::slag doctest::doctest)
if(SLAG_BUILD_RENDERGRAPH)
    find_package(Threads REQUIRED)
    target_sources(slag_backend_tests PRIVATE RenderGraphTests.cpp)
    target_link_libraries(slag_backend_tests PRIVATE slag::rendergraph Threads::Threads)
endif()
doctest_discover_tests(slag_backend_tests PROPERTIES LABELS gpu)

# Shaders for the backend tests (SLAG_SHADER_DIR points at them).
slag_add_shader(slag_backend_tests SOURCE ../shaders/FillBuffer.slang ENTRY main STAGE compute)
slag_add_shader(slag_backend_tests SOURCE ../shaders/DescriptorKinds.slang ENTRY main STAGE compute)
slag_add_shader(slag_backend_tests SOURCE ../shaders/RootConstants.slang ENTRY main STAGE compute)
slag_add_shader(slag_backend_tests SOURCE ../shaders/ReadPoison.slang ENTRY main STAGE compute)
slag_add_shader(slag_backend_tests SOURCE ../shaders/Triangle.slang ENTRY vertexMain STAGE vertex)
slag_add_shader(slag_backend_tests SOURCE ../shaders/Triangle.slang ENTRY pixelMain STAGE pixel)
slag_add_shader(slag_backend_tests SOURCE ../shaders/DrawId.slang ENTRY vertexMain STAGE vertex)
slag_add_shader(slag_backend_tests SOURCE ../shaders/DrawId.slang ENTRY pixelMain STAGE pixel)
slag_add_shader(slag_backend_tests SOURCE ../shaders/FullScreen.slang ENTRY vertexMain STAGE vertex)
slag_add_shader(slag_backend_tests SOURCE ../shaders/Scene.slang ENTRY vertexMain STAGE vertex)
slag_add_shader(slag_backend_tests SOURCE ../shaders/Scene.slang ENTRY pixelMain STAGE pixel)
target_compile_definitions(slag_backend_tests PRIVATE "SLAG_GOLDEN_DIR=\"${PROJECT_SOURCE_DIR}/tests/golden\"")
if(SLAG_BACKEND_D3D12)
    target_compile_definitions(slag_backend_tests PRIVATE SLAG_TEST_AGILITY_SDK_VERSION=${SLAG_AGILITY_SDK_VERSION})
endif()
slag_copy_agility_runtime(slag_backend_tests)
