cmake_minimum_required(VERSION 3.3)

project(common)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter" )

set(SOURCE_FILES memory.c)
add_library(common STATIC ${SOURCE_FILES})
