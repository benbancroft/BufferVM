cmake_minimum_required(VERSION 3.3)
project(buffervm)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set(SOURCE_FILES main.c)
add_executable(buffervm ${SOURCE_FILES})

target_link_libraries(buffervm elf)