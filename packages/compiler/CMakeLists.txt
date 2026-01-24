cmake_minimum_required(VERSION 3.10)
project(cstride)

set(CMAKE_CXX_STANDARD 23)

find_package(LLVM CONFIG REQUIRED)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.cc")
file(GLOB_RECURSE HEADER_FILES "include/*.h" "include/*.hpp")

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# Use target-specific commands
target_include_directories(${PROJECT_NAME} PRIVATE
        "include"
        ${LLVM_INCLUDE_DIRS}
)

# Apply LLVM definitions (like -D__STDC_CONSTANT_MACROS) to the target
target_compile_definitions(${PROJECT_NAME} PRIVATE ${LLVM_DEFINITIONS})

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        LLVMOrcJIT
        LLVMExecutionEngine
        LLVMCore
        LLVMSupport
        LLVMTarget
        LLVMTargetParser
        LLVMRuntimeDyld
        LLVMIRReader
        LLVMAArch64CodeGen
        LLVMAArch64Desc
        LLVMAArch64Info
        LLVMAArch64Utils
        LLVMAArch64AsmParser
)