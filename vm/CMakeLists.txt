cmake_minimum_required(VERSION 3.3)

project(BufferVM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter" )

set(SOURCE_FILES main.c elf.c vm.c vm.h gdt.c)
add_executable(buffervm ${SOURCE_FILES})

add_dependencies(buffervm bootstrap)
target_link_libraries(buffervm common bootstrap elf)
