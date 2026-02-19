cmake_minimum_required(VERSION 4.1.2)

find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB_RECURSE TEST_SOURCES "*.cpp")
file(GLOB_RECURSE TEST_HEADERS "*.h")
add_executable(cstride_tests
    ${TEST_SOURCES}
    ${TEST_HEADERS}
)
target_link_libraries(cstride_tests
    PRIVATE
    GTest::gtest_main
    cstride_lib
)
target_include_directories(cstride_tests PRIVATE
    ../include
    ${LLVM_INCLUDE_DIRS}
)
target_compile_options(cstride_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
target_link_options(cstride_tests PRIVATE -fsanitize=address)
gtest_discover_tests(cstride_tests)
