cmake_minimum_required(VERSION 4.1.2)
cmake_policy(SET CMP0135 NEW)
project(cstride)

set(CMAKE_CXX_STANDARD 23)

option(CSTRIDE_BUILD_ALL_LLVM_TARGETS "Link LLVM backends for multiple targets (enables cross-compiling)" OFF)

# --- Dependencies ---
find_package(LLVM 21.1.8 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

message(STATUS "Retrieving GoogleTest dependencies...")
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

# Forcing GTest to use the same runtime as our project
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- Library Setup ---
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp" "src/*.cc")
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "include/*.h" "include/*.hpp")
list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(cstride_lib STATIC
        ${SOURCE_FILES}
        ${HEADER_FILES}
)

# --- Runtime Library ---
# Always build the runtime optimized and without debug info so that compiled
# user binaries don't embed local build paths or unnecessary C++ template symbols.
add_library(stride_runtime STATIC src/stl/stride_runtime.cpp)
target_compile_options(stride_runtime PRIVATE
    -O2                  # inline chrono templates → far fewer symbol entries
    -g0                  # no DWARF → removes embedded source file paths
    -ffunction-sections  # one section per function, enables dead-code stripping
    -fdata-sections
)

# --- Installation ---
include(GNUInstallDirs)
target_compile_definitions(cstride_lib PRIVATE
    CSTRIDE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
    CSTRIDE_LIB_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/cstride"
    STRIDE_RUNTIME_LIB_PATH="$<TARGET_FILE:stride_runtime>"
)

# --- Executables & Testing ---
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE cstride_lib)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS stride_runtime ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/cstride)

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

set(LLVM_BASE_COMPONENTS
        orcjit executionengine core support
        target targetparser
        mc mcparser
        runtimedyld
        irreader
        asmprinter
        asmparser
)

if(CSTRIDE_BUILD_ALL_LLVM_TARGETS)
    target_compile_definitions(cstride_lib PUBLIC CSTRIDE_ALL_TARGETS=1)
else()
    target_compile_definitions(cstride_lib PUBLIC CSTRIDE_ALL_TARGETS=0)
endif()

if(CSTRIDE_BUILD_ALL_LLVM_TARGETS)
    # All targets
    set(LLVM_TARGET_COMPONENTS
            AArch64CodeGen AArch64Desc AArch64Info AArch64AsmParser
            X86CodeGen X86Desc X86Info X86AsmParser
            ARMCodeGen ARMDesc ARMInfo ARMAsmParser
            RISCVCodeGen RISCVDesc RISCVInfo RISCVAsmParser
            WebAssemblyCodeGen WebAssemblyDesc WebAssemblyInfo WebAssemblyAsmParser
            PowerPCCodeGen PowerPCDesc PowerPCInfo PowerPCAsmParser
            MipsCodeGen MipsDesc MipsInfo MipsAsmParser
    )
else()
    # Host-only
    set(LLVM_TARGET_COMPONENTS
            ${LLVM_NATIVE_ARCH}CodeGen ${LLVM_NATIVE_ARCH}Desc
            ${LLVM_NATIVE_ARCH}Info ${LLVM_NATIVE_ARCH}AsmParser
    )
endif()

llvm_map_components_to_libnames(LLVM_LIBS
        ${LLVM_BASE_COMPONENTS}
        ${LLVM_TARGET_COMPONENTS}
)

target_link_libraries(cstride_lib
        PUBLIC
        ${LLVM_LIBS}
)

# --- Executables & Testing ---
enable_testing()
add_subdirectory(tests)