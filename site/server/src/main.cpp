//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#include <iostream>

#include "app.h"
#include "args.h"
#include "comms.h"
#include "db.h"
#include "logger.h"
#include "util.h"
#include "init.h"
#include "services.h"

int main(int argc, char **argv)
{
    init();
    banner();
    apply_arguments(argc, argv);
    load_services();
    run();
    return 0;
}
