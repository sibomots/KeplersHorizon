##################################################################################
## This file is part of Kepler's Horizon
##
## Licensed under BSD 3-Clause License
##
## Copyright (c) 2025, sibomots
#################################################################################

# Main executable
cc_binary(
    name = "kh",
    srcs = [],
    copts = [
        "-DGIT_SHA=\\\"1e3f502\\\"",
    ],
    includes = ["app/inc"],
    linkopts = ["-lmysqlclient"],
    deps = [
        "//app/combat",
        "//app/internal",
        "//app/map",
        "//app/movement",
        "//app/parser",
        "//app/repres",
        "//app/ships",
    ],
)
