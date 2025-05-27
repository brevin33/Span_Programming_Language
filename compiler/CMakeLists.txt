cmake_minimum_required(VERSION 3.15)
project(Span_Compiler)

add_executable(span_compiler
	src/main.cpp
)

target_include_directories(span_compiler
	PUBLIC
	include
)
