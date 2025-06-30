cmake_minimum_required(VERSION 3.15)
project(Span_Parser C)

set(CMAKE_C_STANDARD 11)

add_library(span_parser
	src/span_parser.c
	src/arena.c
	src/files.c
	src/project.c
	src/logging.c
	src/tokens.c
	src/type.c
)

target_include_directories(span_parser PUBLIC include)
