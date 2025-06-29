cmake_minimum_required(VERSION 3.15)
project(Span_Tests C)

set(CMAKE_C_STANDARD 11)

add_executable(span_tests src/main.c)

target_link_libraries(span_tests span_parser)
