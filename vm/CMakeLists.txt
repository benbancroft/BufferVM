cmake_minimum_required(VERSION 3.3)

project(BufferVM)

enable_language(ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter" )

set(SOURCE_FILES main.c elf.c elf.h memory.c memory.h vm.c vm.h gdt.c gdt.h)
add_executable(buffervm ${SOURCE_FILES})

add_dependencies(buffervm bootstrap)
target_link_libraries(buffervm bootstrap elf)
