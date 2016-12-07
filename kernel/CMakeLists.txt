cmake_minimum_required(VERSION 3.3)

project(kernel C ASM)

include(../common/CMakeLists.txt)

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-pie -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-return-type" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -nostdlib -lgcc -fdata-sections -ffunction-sections" )

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--build-id=none -T ${PROJECT_SOURCE_DIR}/kernel.ld" )

set(SOURCE_FILES start.S c/kernel.c asm/host.s c/idt.c asm/idt.S asm/page_fault.S c/cstubs.c asm/tss.S c/tss.c c/syscall.c asm/syscall.S asm/cpu.S ${COMMON_SOURCE_FILES} c/version.c h/version.h)
add_executable(kernel ${SOURCE_FILES})

target_compile_definitions(kernel PRIVATE KERNEL)

target_link_libraries(kernel c)

set_target_properties(kernel PROPERTIES SUFFIX ".elf")
