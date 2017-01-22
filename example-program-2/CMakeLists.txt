cmake_minimum_required(VERSION 3.3)

project(ExampleProgram)

enable_language(ASM)

set(CMAKE_C_FLAGS "-m64")
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror" )

set(SOURCE_FILES main.c buffervm.S buffervm.h)
add_executable(example-program-2 ${SOURCE_FILES})
