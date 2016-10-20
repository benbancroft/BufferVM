cmake_minimum_required(VERSION 3.3)

project(libc)

enable_language(ASM)

set(CMAKE_C_FLAGS "-g -w -m64 -nostdlib -mcmodel=large")
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-return-type" )

set(SOURCE_FILES stdlib.c)
add_library(c STATIC ${SOURCE_FILES})
