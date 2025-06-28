cmake_minimum_required(VERSION 3.15)

add_executable(span_simple_example
	simple.c
)

target_link_libraries(span_simple_example
	PRIVATE
		span_parser
)
