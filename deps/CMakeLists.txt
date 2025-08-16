cmake_minimum_required(VERSION 3.15)
project(Span_Parser CXX)

set(CMAKE_C_STANDARD 11)


find_package(LLVM CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs Support Core IRReader)

add_library(span_deps
	deps.cpp
)

target_include_directories(span_deps
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(span_deps
	PUBLIC
        ${LLVM_DEFINITIONS}
)

# Link with LLVM C API libraries
target_link_libraries(span_deps
	PUBLIC
	    ${llvm_libs}
		LLVM-C
)
