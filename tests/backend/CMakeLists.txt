# Backend tests: need a real (or software) GPU, so they carry the "gpu" label.
# Hosted CI excludes them (ctest -LE gpu); the self-hosted GPU runner runs them.

add_executable(slag_backend_tests
    BackendTestMain.cpp
    InstanceTests.cpp)
target_link_libraries(slag_backend_tests PRIVATE slag::slag doctest::doctest)
doctest_discover_tests(slag_backend_tests PROPERTIES LABELS gpu)
