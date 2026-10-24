# Validation checks (ADR 0067), without a GPU: Slag's common code built
# again, always with validation, around a null backend that only creates
# objects. Never linked with the real library.

set(_registry "${CMAKE_CURRENT_BINARY_DIR}/NullBackendRegistry.cpp")
set(SLAG_BACKEND_DECLARATIONS "const BackendInfo* slagBackend_null();\n")
set(SLAG_BACKEND_ENTRIES "        slagBackend_null(),\n")
configure_file("${PROJECT_SOURCE_DIR}/cmake/BackendRegistry.cpp.in" "${_registry}" @ONLY)

add_library(slag_null STATIC ${SLAG_COMMON_SOURCES} NullBackend.cpp "${_registry}")
target_compile_features(slag_null PUBLIC cxx_std_20)
target_include_directories(slag_null PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${SLAG_GENERATED_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(slag_null PUBLIC SLAG_VALIDATION SLAG_VALIDATION_COVERAGE)
target_compile_definitions(slag_null PRIVATE SLAG_DIRECT_BACKEND_NAMESPACE=null)

add_executable(slag_validation_tests
    ValidationTestMain.cpp
    DescriptorValidation.cpp
    DeviceValidation.cpp
    CommandValidation.cpp
    QueryLabelValidation.cpp
    RenderingValidation.cpp
    ResourceValidation.cpp
    SwapchainValidation.cpp)
target_link_libraries(slag_validation_tests PRIVATE slag_null doctest::doctest)
target_compile_definitions(slag_validation_tests PRIVATE "SLAG_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}/src\"")
doctest_discover_tests(slag_validation_tests)
# Every check in src/ fires in some test (M7 "done when"): one run of all
# tests, then the coverage report.
add_test(NAME validation_coverage COMMAND slag_validation_tests --coverage)
