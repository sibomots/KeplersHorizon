##################################################################################
## This file is part of Kepler's Horizon
##
## Licensed under BSD 3-Clause License
##
## Copyright (c) 2025, sibomots
#################################################################################

# Collect all .cpp files in this directory
file(GLOB INTERNAL_SRCS "*.cpp")

# Add sources to parent scope
set(INTERNAL_SRCS ${INTERNAL_SRCS} PARENT_SCOPE)
