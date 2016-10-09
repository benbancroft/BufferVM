cmake_minimum_required(VERSION 3.3)

SET(CMAKE_C_LINK_EXECUTABLE "ld")

project(bootloader ASM)

set(SOURCE_FILES bootloader.s)
add_library(bootloader.o OBJECT ${SOURCE_FILES})

SET(BOOTLOADER_S_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/bootloader)
SET(BOOTLOADER_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/bootloader.o)

add_custom_command(OUTPUT ${BOOTLOADER_OBJECT}
        COMMAND ${CMAKE_C_LINK_EXECUTABLE} -T ${CMAKE_SOURCE_DIR}/bootloader/bootloader.ld ${BOOTLOADER_S_OBJECT} -o ${BOOTLOADER_OBJECT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS bootloader.o)

add_library(bootloader
        STATIC
        ${BOOTLOADER_OBJECT})

SET_TARGET_PROPERTIES(
        bootloader
        PROPERTIES
        LINKER_LANGUAGE C
)