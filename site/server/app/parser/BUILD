##################################################################################
## This file is part of Kepler's Horizon
##
## Licensed under BSD 3-Clause License
##
## Copyright (c) 2025, sibomots
#################################################################################

# Bison parser
genrule(
    name = "parser_gen",
    srcs = ["lang.y"],
    outs = [
        "lang.cpp",
        "lang.hpp",
    ],
    cmd = "bison -d -v -g -o $(location lang.cpp) $<",
)

# Flex scanner
genrule(
    name = "scanner_gen",
    srcs = ["scan.lex"],
    outs = ["scan.cpp"],
    cmd = "flex -o $(location scan.cpp) $<",
)

cc_library(
    name = "parser",
    srcs = [
        ":parser_gen",
        ":scanner_gen",
    ],
    hdrs = ["//app/inc:headers"],
    includes = ["../../app/inc"],
    visibility = ["//visibility:public"],
)
