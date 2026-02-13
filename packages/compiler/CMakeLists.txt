cmake_minimum_required(VERSION 4.1.2)
cmake_policy(SET CMP0135 NEW)
project(cstride)

set(CMAKE_CXX_STANDARD 23)

# --- Dependencies ---
find_package(LLVM 21.1.8 REQUIRED CONFIG)

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
    set(ARCH_DEPENDENT_LIBS
            LLVMAArch64CodeGen LLVMAArch64Desc LLVMAArch64Info
            LLVMAArch64Utils LLVMAArch64AsmParser)
else()
    set(ARCH_DEPENDENT_LIBS
            LLVMX86CodeGen LLVMX86Desc LLVMX86Info
            LLVMX86Utils LLVMX86AsmParser LLVMX86AsmPrinter)
endif()

target_link_libraries(cstride_lib
        PUBLIC
        LLVMOrcJIT LLVMExecutionEngine LLVMCore LLVMSupport
        LLVMTarget LLVMTargetParser LLVMRuntimeDyld LLVMIRReader
        LLVMAsmPrinter
        ${ARCH_DEPENDENT_LIBS}
)

# --- Executables & Testing ---
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE cstride_lib)

enable_testing()
add_subdirectory(tests)