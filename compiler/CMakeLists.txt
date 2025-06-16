cmake_minimum_required(VERSION 3.15)
project(Span_Compiler)

add_executable(span_compiler
	src/main.c
	src/compiler.c
	src/project.c
	src/function.c
	src/type.c
	src/scope.c
)


find_package(LLVM REQUIRED CONFIG)

target_link_libraries(span_compiler
	PRIVATE
	span_parser
    LLVM-C
)


target_include_directories(span_compiler
	PUBLIC
	include
    ${LLVM_INCLUDE_DIRS}
)
