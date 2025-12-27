###################################################################################
## BSD 3-Clause License
## 
## This file is part of Kepler's Horizon
##
## Copyright (c) 2025, sibomots
## 
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
## 
## 1. Redistributions of source code must retain the above copyright notice, this
##    list of conditions and the following disclaimer.
## 
## 2. Redistributions in binary form must reproduce the above copyright notice,
##    this list of conditions and the following disclaimer in the documentation
##    and/or other materials provided with the distribution.
## 
## 3. Neither the name of the copyright holder nor the names of its
##    contributors may be used to endorse or promote products derived from
##    this software without specific prior written permission.
## 
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
## DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
## DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
## SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
## CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
## OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#################################################################################

cmake_minimum_required(VERSION 3.10)

project(kh LANGUAGES CXX)

# Match original Makefile intent
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Equivalent to: CXX_FLAGS = -O2 -pedantic
add_compile_options(-O2 -pedantic)

# Source files
set(SRCS
src/args.cpp
src/game.cpp
src/util.cpp
src/events.cpp
src/comms.cpp
src/cmd.cpp
src/db.cpp
src/state.cpp
src/login.cpp
src/logout.cpp
src/main.cpp
src/json.cpp
)

# Header files (not required for build, but useful for IDEs)
set(HDRS
inc/util.h
inc/args.h
inc/state.h
inc/db.h
inc/typs.h
inc/game.h
inc/json.h
inc/events.h
inc/cmd.h
inc/app.h
inc/comms.h


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

