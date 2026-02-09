cmake_minimum_required(VERSION 3.10)
project(cstride)

set(CMAKE_CXX_STANDARD 23)

find_package(LLVM CONFIG REQUIRED)

enable_testing()

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.cc")
file(GLOB_RECURSE HEADER_FILES "include/*.h" "include/*.hpp")

list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(cstride_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(cstride_lib PUBLIC
        "include"
        ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(cstride_lib PUBLIC ${LLVM_DEFINITIONS})

target_link_libraries(cstride_lib
        PUBLIC
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

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} cstride_lib)

add_subdirectory(tests)
