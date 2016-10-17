cmake_minimum_required(VERSION 3.3)

project(kernel C ASM)

#set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -nostdlib -lgcc -fdata-sections -ffunction-sections" )


set(CMAKE_EXE_LINKER_FLAGS "-Wl,--build-id=none -T${PROJECT_SOURCE_DIR}/kernel.ld" )

set(SOURCE_FILES start.S c/kernel.c asm/host.s c/idt.c asm/idt.s asm/page_fault.s c/cstubs.c asm/tss.S c/tss.c)
add_executable(kernel ${SOURCE_FILES})

target_link_libraries(kernel c)

set_target_properties(kernel PROPERTIES SUFFIX ".elf")