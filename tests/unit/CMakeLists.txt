# Header self-containment: every public header must compile on its own
# (CLAUDE.md §10). One generated translation unit per header.

file(GLOB_RECURSE _slag_public_headers CONFIGURE_DEPENDS
    RELATIVE "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/slag/*.h")

# The generated version header is checked too.
list(APPEND _slag_public_headers "slag/Version.h")

# The native interop headers pull in platform SDKs and are checked by backend
# builds. native/D3D12Agility.h needs no SDK, so it stays in.
list(FILTER _slag_public_headers EXCLUDE REGEX "^slag/native/(D3D12|Vulkan)\\.h$")

set(_slag_header_test_sources)
foreach(_header IN LISTS _slag_public_headers)
    string(MAKE_C_IDENTIFIER "${_header}" _id)
    set(_tu "${CMAKE_CURRENT_BINARY_DIR}/header_check/${_id}.cpp")
    file(CONFIGURE OUTPUT "${_tu}" CONTENT "#include <${_header}>\n")
    list(APPEND _slag_header_test_sources "${_tu}")
endforeach()

add_executable(slag_header_check HeaderCheckMain.cpp ${_slag_header_test_sources})
target_link_libraries(slag_header_check PRIVATE slag::slag)
add_test(NAME header_check COMMAND slag_header_check)

# Unit tests (doctest; tests only, ADR 0061).
add_executable(slag_unit_tests
    UnitTestMain.cpp
    FlagsTests.cpp
    InstanceTests.cpp
    ResultTests.cpp
    VectorTests.cpp
    VersionTests.cpp)
target_link_libraries(slag_unit_tests PRIVATE slag::slag doctest::doctest)
# Unit tests may test internal code directly.
target_include_directories(slag_unit_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
doctest_discover_tests(slag_unit_tests)
