//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include "init.h"
#include "srvmgr.h"

int main(int argc, char** argv)
{
    banner();
    apply_arguments(argc, argv);
    init();
    load_services();

    // Now we run...
    ServerManager::getInstance().run();

    // We never get here
    return 0;
}
