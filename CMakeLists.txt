cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME cstride)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 23)

# # # # # # # # # # # # # # # #
#          LLVM Setup         #
# # # # # # # # # # # # # # # #
find_package(LLVM CONFIG)

if (LLVM_FOUND)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

    # Include directories and definitions
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})

    # Link against specific LLVM libraries
    llvm_map_components_to_libnames(llvm_libs support core irreader)
else ()
    message(STATUS "LLVM not found - building without LLVM support")
endif ()

# # # # END LLVM Setup # # # #

file(GLOB_RECURSE HEADER_FILES "./include/*.h" "./include/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "./src/*.cpp" "./src/*.cc")


list(LENGTH HEADER_FILES HEADER_COUNT)
list(LENGTH SOURCE_FILES SOURCE_COUNT)
math(EXPR TOTAL_FILES "${HEADER_COUNT} + ${SOURCE_COUNT}")
message(STATUS "Building ${PROJECT_NAME}: ${TOTAL_FILES} file(s) in total (${HEADER_COUNT} headers, ${SOURCE_COUNT} sources)")

include_directories(./include)

add_executable(${PROJECT_NAME}
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

if (LLVM_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${llvm_libs})
endif ()
