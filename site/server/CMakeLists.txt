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

BISON_TARGET(Parser src/lang.y ${CMAKE_CURRENT_BINARY_DIR}/lang.cpp COMPILE_FLAGS "-d -v -g")
FLEX_TARGET(Scanner src/scan.lex ${CMAKE_CURRENT_BINARY_DIR}/scan.cpp)
ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)


# Source files
set(SRCS
src/Build.cpp
src/Combat.cpp
src/Move.cpp
src/RepRes.cpp
src/Turn.cpp
src/account.cpp
src/cmd.cpp
src/comms.cpp
src/db.cpp
src/events.cpp
src/fleet_command.cpp
src/system_command.cpp
src/survey_command.cpp
src/score_command.cpp
src/extract_command.cpp
src/help_command.cpp
src/market_command.cpp
src/trade_command.cpp
src/fabricate_command.cpp
src/constraints.cpp
src/salvage_command.cpp
src/facilities.cpp
src/turn_end.cpp
src/init.cpp
src/json.cpp
src/logger.cpp
src/main.cpp
src/mapgraph.cpp
src/maputil.cpp
src/roommanager.cpp
src/rooms_api.cpp
src/save_command.cpp
src/ships.cpp
src/srvmgr.cpp
src/star_system_constraints.cpp
src/state.cpp
src/statemachine.cpp
src/telemetry.cpp
src/util.cpp
${BISON_Parser_OUTPUTS}
${FLEX_Scanner_OUTPUTS}
)

# Header files (not required for build, but useful for IDEs)
set(HDRS
inc/app.h
inc/build_cancel_command.h
inc/build_command.h
inc/build_commit_command.h
inc/build_list_drafts_command.h
inc/build_new_command.h
inc/build_set_attribute_command.h
inc/build_show_draft_command.h
inc/cmd.h
inc/combat.h
inc/comms.h
inc/db.h
inc/deploy_command.h
inc/done_command.h
inc/events.h
inc/ships.h
inc/icmd.h
inc/init.h
inc/json.h
inc/logger.h
inc/mapgraph.h
inc/maputil.h
inc/move_command.h
inc/next_command.h
inc/repair_command.h
inc/resupply_command.h
inc/roommanager.h
inc/ships.h
inc/srvmgr.h
inc/start_command.h
inc/system_command.h
inc/survey_command.h
inc/score_command.h
inc/extract_command.h
inc/help_command.h
inc/market_command.h
inc/trade_command.h
inc/fabricate_command.h
inc/constraints.h
inc/salvage_command.h
inc/facilities.h
inc/turn_end.h
inc/state.h
inc/statemachine.h
inc/telemetry.h
inc/typedefs.h
inc/util.h
)

add_executable(kh
    ${SRCS}
    ${HDRS}
)

# Make headers visible to all source files
target_include_directories(kh
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Link MySQL client (exact Makefile equivalent)
target_link_libraries(kh
    mysqlclient
)

