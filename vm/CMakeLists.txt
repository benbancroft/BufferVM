cmake_minimum_required(VERSION 3.3)

project(BufferVM)

enable_language(ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
#set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror" )

#add_custom_target(bootloader bootloader.s)

set(SOURCE_FILES main.c elf.c elf.h memory.c memory.h vm.c vm.h)
add_executable(buffervm ${SOURCE_FILES})

add_dependencies(buffervm bootloader)
target_link_libraries(buffervm bootloader elf)
