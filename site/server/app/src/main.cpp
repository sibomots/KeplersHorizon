//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////

#include "init.h"
#include "srvmgr.h"
#include "db.h"

int main(int argc, char** argv)
{
    banner();
    mysql_library_init(0, NULL, NULL);
    apply_arguments(argc, argv);
    init();
    load_services();

    // Now we run...
    ServerManager::instance().run();
    mysql_library_end();
    // We never get here
    return 0;
}
