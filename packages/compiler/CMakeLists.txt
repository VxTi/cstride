cmake_minimum_required(VERSION 4.1.2) # Higher version recommended for FetchContent
cmake_policy(SET CMP0135 NEW) # Silences the warning and sets modern behavior
project(cstride)

set(CMAKE_CXX_STANDARD 23)

# --- 1. Dependencies ---
find_package(LLVM 21.1.8 REQUIRED CONFIG)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# GoogleTest setup
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# Forcing GTest to use the same runtime as our project
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- 2. Flex & Bison Generation ---
# Define paths for generated files as requested by the issue
set(BISON_OUTPUT_CPP "${CMAKE_CURRENT_SOURCE_DIR}/src/Stride.tab.cpp")
set(BISON_OUTPUT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/Stride.tab.h")
set(FLEX_OUTPUT_CPP "${CMAKE_CURRENT_SOURCE_DIR}/src/Stride.lex.cpp")

BISON_TARGET(StrideParser grammar/Stride.y ${BISON_OUTPUT_CPP} DEFINES_FILE ${BISON_OUTPUT_HEADER})
FLEX_TARGET(StrideLexer grammar/Stride.l ${FLEX_OUTPUT_CPP})
ADD_FLEX_BISON_DEPENDENCY(StrideLexer StrideParser)

# --- 3. Library Setup ---
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.cc")
file(GLOB_RECURSE HEADER_FILES "include/*.h" "include/*.hpp")
list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(cstride_lib STATIC
        ${SOURCE_FILES}
        ${HEADER_FILES}
        ${BISON_StrideParser_OUTPUTS}
        ${FLEX_StrideLexer_OUTPUTS}
)

# --- 4. Include Management ---
# Automatically find all header directories within _deps
file(GLOB_RECURSE DEPS_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/_deps/*")
foreach(_dir ${DEPS_INCLUDE_DIRS})
    if(IS_DIRECTORY ${_dir})
        target_include_directories(cstride_lib SYSTEM PUBLIC ${_dir})
    endif()
endforeach()

target_include_directories(cstride_lib PUBLIC
        "include"
        ${CMAKE_CURRENT_BINARY_DIR} # Critical for finding generated parser.hpp
        ${LLVM_INCLUDE_DIRS}
)

# --- 5. Linking & Definitions ---
target_compile_definitions(cstride_lib PUBLIC ${LLVM_DEFINITIONS})

target_link_libraries(cstride_lib
        PUBLIC
        LLVMOrcJIT LLVMExecutionEngine LLVMCore LLVMSupport
        LLVMTarget LLVMTargetParser LLVMRuntimeDyld LLVMIRReader
        LLVMAArch64CodeGen LLVMAArch64Desc LLVMAArch64Info
        LLVMAArch64Utils LLVMAArch64AsmParser
)

# --- 6. Executables & Testing ---
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE cstride_lib)

enable_testing()
add_subdirectory(tests)