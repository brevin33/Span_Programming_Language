cmake_minimum_required(VERSION 3.15)
project(Span_Parser CXX)

set(CMAKE_C_STANDARD 11)

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
)

target_compile_definitions(span_parser 
	PUBLIC
)

# Link with LLVM C API libraries
target_link_libraries(span_parser 
	PUBLIC
	    span_deps
)
