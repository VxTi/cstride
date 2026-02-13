cmake_minimum_required(VERSION 4.1.2)
cmake_policy(SET CMP0135 NEW)
project(cstride)

set(CMAKE_CXX_STANDARD 23)

# --- Dependencies ---
find_package(LLVM 21.1.8 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# GoogleTest setup
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# Forcing GTest to use the same runtime as our project
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- Library Setup ---
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.cc")
file(GLOB_RECURSE HEADER_FILES "include/*.h" "include/*.hpp")
list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(cstride_lib STATIC
        ${SOURCE_FILES}
        ${HEADER_FILES}
)

# --- Include Management ---
# Automatically find all header directories within _deps
file(GLOB_RECURSE DEPS_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/_deps/*")
foreach(_dir ${DEPS_INCLUDE_DIRS})
    if(IS_DIRECTORY ${_dir})
        target_include_directories(cstride_lib SYSTEM PUBLIC ${_dir})
    endif()
endforeach()

target_include_directories(cstride_lib PUBLIC
        "include"
        ${CMAKE_CURRENT_BINARY_DIR}
        ${LLVM_INCLUDE_DIRS}
)

# --- Linking & Definitions ---
target_compile_definitions(cstride_lib PUBLIC ${LLVM_DEFINITIONS})

# Determine architecture to link correct LLVM backend libraries for native target support
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(LLVM_NATIVE_ARCH AArch64)
else()
    set(LLVM_NATIVE_ARCH X86)
endif()

llvm_map_components_to_libnames(LLVM_LIBS
        orcjit executionengine core support
        target targetparser runtimedyld irreader
        asmprinter
        ${LLVM_NATIVE_ARCH}CodeGen ${LLVM_NATIVE_ARCH}Desc
        ${LLVM_NATIVE_ARCH}Info ${LLVM_NATIVE_ARCH}AsmParser
)

target_link_libraries(cstride_lib
        PUBLIC
        ${LLVM_LIBS}
)

# --- Executables & Testing ---
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE cstride_lib)

enable_testing()
add_subdirectory(tests)