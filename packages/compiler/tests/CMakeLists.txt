cmake_minimum_required(VERSION 3.10)
include(GoogleTest)
file(GLOB_RECURSE TEST_SOURCES "*.cpp")
file(GLOB_RECURSE TEST_HEADERS "*.h")
add_executable(cstride_tests
    ${TEST_SOURCES}
    ${TEST_HEADERS}
)
target_link_libraries(cstride_tests
    PRIVATE
    gtest_main
    cstride_lib
)
target_include_directories(cstride_tests PRIVATE
    ../include
    ${LLVM_INCLUDE_DIRS}
)
gtest_discover_tests(cstride_tests)
