cmake_minimum_required(VERSION 3.3)

project(ExampleProgram)

enable_language(ASM)

set(CMAKE_C_FLAGS "-g -w -m64 -nostdlib")
#SET(CMAKE_ASM_FLAGS "-m32")

set(SOURCE_FILES sbrk.c alloc.c main.c start.s cstubs.s)
add_executable(example-program ${SOURCE_FILES})

target_link_libraries(example-program c)
