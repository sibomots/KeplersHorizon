##################################################################################
## This file is part of Kepler's Horizon
##
## Licensed under BSD 3-Clause License
##
## Copyright (c) 2025, sibomots
#################################################################################
cmake_minimum_required(VERSION 3.10)

project(kh LANGUAGES CXX)

# Match original Makefile intent
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-O0 -pedantic -g)

# Get Git SHA
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT GIT_SHA)
    set(GIT_SHA "unknown")
endif()
add_definitions(-DGIT_SHA="${GIT_SHA}")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# Generate parser files (must be done before add_subdirectory)
BISON_TARGET(Parser 
    ${CMAKE_CURRENT_SOURCE_DIR}/app/parser/lang.y 
    ${CMAKE_CURRENT_BINARY_DIR}/app/parser/lang.cpp 
    COMPILE_FLAGS "-d -v -g"
)
FLEX_TARGET(Scanner 
    ${CMAKE_CURRENT_SOURCE_DIR}/app/parser/scan.lex 
    ${CMAKE_CURRENT_BINARY_DIR}/app/parser/scan.cpp
)
ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

# Include subdirectories to collect sources
add_subdirectory(app/inc)
add_subdirectory(app/internal)
add_subdirectory(app/ships)
add_subdirectory(app/movement)
add_subdirectory(app/combat)
add_subdirectory(app/repres)
add_subdirectory(app/map)

# Combine all sources
set(ALL_SRCS
    ${INTERNAL_SRCS}
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Scanner_OUTPUTS}
    ${SHIPS_SRCS}
    ${MOVEMENT_SRCS}
    ${COMBAT_SRCS}
    ${REPRES_SRCS}
    ${MAP_SRCS}
)

# Create executable
add_executable(kh
    ${ALL_SRCS}
    ${INC_HDRS}
)

# Make headers visible to all source files
target_include_directories(kh
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app/inc
        ${CMAKE_CURRENT_BINARY_DIR}/app/parser
)

# Link MySQL client (exact Makefile equivalent)
target_link_libraries(kh
    mysqlclient
)


