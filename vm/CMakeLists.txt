cmake_minimum_required(VERSION 3.3)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set(SOURCE_FILES main.c elf.c elf.h memory.c memory.h vm.c vm.h)
add_executable(buffervm ${SOURCE_FILES})

target_link_libraries(buffervm elf)
