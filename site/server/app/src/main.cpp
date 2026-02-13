///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#include <csignal>

#include "db.h"
#include "init.h"
#include "srvmgr.h"

void signal_handler(int signum)
{
    (void)signum;
    ServerManager::request_shutdown();
}

int main(int argc, char** argv)
{
    pre_init();
    banner();
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    mysql_library_init(0, NULL, NULL);
    apply_arguments(argc, argv);
    init();
    load_services();

    // Now we run...
    ServerManager::instance().run();
    mysql_library_end();
    return 0;
}
