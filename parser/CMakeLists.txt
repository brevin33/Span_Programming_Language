cmake_minimum_required(VERSION 3.15)
project(Span_Parser)

SET(CMAKE_CXX_STANDARD 17)

add_library(span_parser
	src/project.cpp
	src/logging.cpp
	src/tokens.cpp
)

target_include_directories(span_parser
	PUBLIC
	include
)

add_subdirectory(examples)
