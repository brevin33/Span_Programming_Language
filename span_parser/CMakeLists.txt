cmake_minimum_required(VERSION 3.15)
project(Span_Parser C)

set(CMAKE_C_STANDARD 11)

add_executable(span_parser src/span_parser.c)

target_include_directories(span_parser PUBLIC include)
