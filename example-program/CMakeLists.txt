cmake_minimum_required(VERSION 3.3)
enable_language(ASM)

set(CMAKE_C_FLAGS "-g -w -m32 -nostdlib")
SET(CMAKE_ASM_FLAGS "-m32")

set(SOURCE_FILES sbrk.c alloc.c stdlib.c main.c stubstart.s)
add_executable(example-program ${SOURCE_FILES})
