##################################################################################
## This file is part of Kepler's Horizon
##
## Licensed under BSD 3-Clause License
##
## Copyright (c) 2025, sibomots
#################################################################################

# Collect all .cpp files in this directory
file(GLOB SHIPS_SRCS "*.cpp")

# Add sources to parent scope
set(SHIPS_SRCS ${SHIPS_SRCS} PARENT_SCOPE)
