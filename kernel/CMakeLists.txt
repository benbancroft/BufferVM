cmake_minimum_required(VERSION 3.3)

project(kernel C ASM)

#set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -mcmodel=large -nostdlib -lgcc -fdata-sections -ffunction-sections" )


set(CMAKE_EXE_LINKER_FLAGS "-Wl,--build-id=none -T${PROJECT_SOURCE_DIR}/kernel.ld" )

set(SOURCE_FILES start.s c/kernel.c asm/host.s c/idt.c asm/idt.s c/cstubs.c c/gdt.c asm/gdt.s)
add_executable(kernel ${SOURCE_FILES})

target_link_libraries(kernel c)

set_target_properties(kernel PROPERTIES SUFFIX ".elf")