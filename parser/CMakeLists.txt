cmake_minimum_required(VERSION 3.15)
project(Span_Parser)

SET(CMAKE_CXX_STANDARD 17)

add_library(span_parser
	src/project.c
	src/logging.c
	src/arena.c
	src/tokens.c
	src/files.c
	src/map.c
	src/pool.c
	src/parser.c
	src/sourceCode.c
	src/project.c
	src/type.c
	src/function.c
	src/template.c
	src/scope.c
	src/statment.c
	src/expression.c
	src/utils.c
)

target_include_directories(span_parser
	PUBLIC
	include
)

add_subdirectory(examples)
