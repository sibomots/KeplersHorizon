##################################################################################
## This file is part of Kepler's Horizon
##
## Licensed under BSD 3-Clause License
##
## Copyright (c) 2025, sibomots
#################################################################################

# Collect all .h files in this directory
file(GLOB INC_HDRS "*.h")

# Add headers to parent scope
set(INC_HDRS ${INC_HDRS} PARENT_SCOPE)
