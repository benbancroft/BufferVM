cmake_minimum_required(VERSION 3.3)

SET(CMAKE_C_LINK_EXECUTABLE "ld")

project(intelxed C)

SET(INTELXED_LIB ${CMAKE_CURRENT_SOURCE_DIR}/kit/lib/libxed.a)

add_custom_command(OUTPUT ${INTELXED_LIB}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/xed/mfile.py install --install-dir ${CMAKE_CURRENT_SOURCE_DIR}/kit --extra-ccflags='-mcmodel=large -fno-stack-protector -mno-red-zone  -fdata-sections -ffunction-sections'
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(intelxed
        COMMAND patch -p0 -f -r - < ${CMAKE_CURRENT_SOURCE_DIR}/xed.patch || true
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${INTELXED_LIB})