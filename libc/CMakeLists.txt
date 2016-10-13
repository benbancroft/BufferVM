cmake_minimum_required(VERSION 3.3)

project(libc)

enable_language(ASM)

set(CMAKE_C_FLAGS "-g -w -m64 -nostdlib")

set(SOURCE_FILES stdlib.c)
add_library(c STATIC ${SOURCE_FILES})
