cmake_minimum_required(VERSION 3.15)
project(Span_Programming_Language)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MT")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MT")

add_subdirectory(deps)
add_subdirectory(span_parser)
add_subdirectory(tests)


