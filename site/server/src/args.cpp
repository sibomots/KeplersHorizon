//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "typs.h"

Args parse_args(int argc, char **argv)
{
    Args a;
    for (int i = 1; i < argc; i++)
    {
        std::string k = argv[i];
        auto next = [&](std::string &out)
        {
            if (i + 1 >= argc)
                throw std::runtime_error("missing arg for " + k);
            out = argv[++i];
        };
        if (k == "--dbhost")
            next(a.dbhost);
        else if (k == "--dbuser")
            next(a.dbuser);
        else if (k == "--dbpass")
            next(a.dbpass);
        else if (k == "--dbname")
            next(a.dbname);
        else if (k == "--listen")
            next(a.listen);
        else if (k == "--port")
        {
            std::string t;
            next(t);
            a.port = std::atoi(t.c_str());
        }
    }
    return a;
}
