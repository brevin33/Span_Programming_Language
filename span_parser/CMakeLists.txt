cmake_minimum_required(VERSION 3.15)
project(Span_Parser C)

set(CMAKE_C_STANDARD 11)

# Include FetchContent module
include(FetchContent)

# Set the host triple explicitly for Windows
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LLVM_HOST_TRIPLE "x86_64-pc-windows-msvc" CACHE STRING "Host triple")
    else()
        set(LLVM_HOST_TRIPLE "i686-pc-windows-msvc" CACHE STRING "Host triple")
    endif()
endif()

# Fetch LLVM
FetchContent_Declare(
    llvm
    GIT_REPOSITORY https://github.com/llvm/llvm-project.git
    GIT_TAG        llvmorg-18.1.8  # Use a stable release tag
    SOURCE_SUBDIR  llvm
)

# Set LLVM options before fetching
set(LLVM_ENABLE_PROJECTS "" CACHE STRING "")
set(LLVM_TARGETS_TO_BUILD "X86" CACHE STRING "Build targets for LLVM")
set(LLVM_BUILD_EXAMPLES OFF CACHE BOOL "")
set(LLVM_BUILD_TESTS OFF CACHE BOOL "")
set(LLVM_BUILD_TOOLS OFF CACHE BOOL "")
set(LLVM_BUILD_UTILS OFF CACHE BOOL "")
set(LLVM_INCLUDE_EXAMPLES OFF CACHE BOOL "")
set(LLVM_INCLUDE_TESTS OFF CACHE BOOL "")
set(LLVM_INCLUDE_TOOLS OFF CACHE BOOL "")
set(LLVM_INCLUDE_UTILS OFF CACHE BOOL "")
set(LLVM_ENABLE_ASSERTIONS OFF CACHE BOOL "")
set(LLVM_ENABLE_RTTI ON CACHE BOOL "")
set(LLVM_ENABLE_EH ON CACHE BOOL "")
set(LLVM_INCLUDE_BENCHMARKS OFF CACHE BOOL "")
set(LLVM_ENABLE_LIBXML2 OFF CACHE BOOL "")
set(LLVM_ENABLE_ZLIB OFF CACHE BOOL "")
set(LLVM_ENABLE_ZSTD OFF CACHE BOOL "")

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lws2_32 -lwsock32" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lws2_32 -lwsock32" CACHE STRING "" FORCE)
endif()

# Make LLVM available
FetchContent_MakeAvailable(llvm)

# Find LLVM components you need
find_package(LLVM REQUIRED CONFIG PATHS ${llvm_BINARY_DIR} NO_DEFAULT_PATH)

add_library(span_parser
	src/span_parser.c
	src/arena.c
	src/files.c
	src/project.c
	src/logging.c
	src/tokens.c
	src/type.c
	src/utils.c
	src/ast.c
	src/function.c
	src/expression.c
	src/statment.c
	src/scope.c
)

target_include_directories(span_parser 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(span_parser 
	PUBLIC
        ${LLVM_DEFINITIONS}
)

# Link with LLVM C API libraries
target_link_libraries(span_parser 
	PUBLIC
        LLVM-C
)
